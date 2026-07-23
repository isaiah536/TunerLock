import fs from "node:fs/promises";
import path from "node:path";
import { SpreadsheetFile, Workbook } from "@oai/artifact-tool";

const root = "C:/Users/Yeo HyeonSeo/TunerLock/TunerLock";
const outputDir = `${root}/outputs/cello_scale_test`;
const correctedCsvPath =
  `${root}/tracking_engine/build/cello_scale_test_engine_frames_low_memory_fix.csv`;
const previousCsvPath =
  `${root}/tracking_engine/build/cello_scale_test_engine_frames.csv`;
const octaveCsvPath =
  `${root}/tracking_engine/build/cello_scale_frame_features_with_octaves.csv`;
const outputPath = `${outputDir}/cello_scale_test_analysis.xlsx`;

const notes = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];

function parseCsv(text) {
  const lines = text.trim().split(/\r?\n/);
  const headers = lines.shift().split(",");
  return lines
    .filter((line) => line.trim().length > 0)
    .map((line) => {
      const values = line.split(",");
      const row = {};
      headers.forEach((header, index) => {
        row[header] = values[index] ?? "";
      });
      return row;
    });
}

function number(value) {
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : 0;
}

function noteName(frequencyHz) {
  if (frequencyHz <= 0) {
    return "None";
  }
  const midi = Math.round(69 + 12 * Math.log2(frequencyHz / 440));
  const octave = Math.floor(midi / 12) - 1;
  const note = notes[((midi % 12) + 12) % 12];
  return `${note}${octave}`;
}

function enrichedFrames(rows) {
  return rows.map((row) => {
    const frequencyHz = number(row.frequency_hz);
    return {
      frame: number(row.frame),
      timeSeconds: number(row.time_seconds),
      frequencyHz,
      cents: number(row.cents),
      confidence: number(row.confidence),
      locked: row.locked === "1" || row.locked === "true",
      note: noteName(frequencyHz),
    };
  });
}

function summarize(frames) {
  const lockedFrames = frames.filter((row) => row.locked);
  const validFrames = frames.filter((row) => row.frequencyHz > 0);
  const confidences = frames.map((row) => row.confidence);
  const frequencies = validFrames.map((row) => row.frequencyHz);
  const average = (values) =>
    values.length ? values.reduce((sum, value) => sum + value, 0) / values.length : 0;
  return {
    frameCount: frames.length,
    lockedFrameCount: lockedFrames.length,
    firstLockedFrame: lockedFrames.length ? lockedFrames[0].frame : null,
    confidenceMean: average(confidences),
    confidenceMin: Math.min(...confidences),
    confidenceMax: Math.max(...confidences),
    frequencyMeanHz: average(frequencies),
  };
}

function segments(frames) {
  if (!frames.length) {
    return [];
  }
  const output = [];
  let start = 0;
  let current = frames[0].note;
  const pushSegment = (endExclusive) => {
    const slice = frames.slice(start, endExclusive);
    if (slice.length < 3) {
      return;
    }
    const average = (values) =>
      values.length ? values.reduce((sum, value) => sum + value, 0) / values.length : 0;
    output.push({
      note: current,
      startSeconds: slice[0].timeSeconds,
      endSeconds: slice[slice.length - 1].timeSeconds,
      frames: slice.length,
      meanHz: average(slice.map((row) => row.frequencyHz)),
      meanConfidence: average(slice.map((row) => row.confidence)),
    });
  };

  for (let i = 1; i < frames.length; i += 1) {
    if (frames[i].note !== current) {
      pushSegment(i);
      start = i;
      current = frames[i].note;
    }
  }
  pushSegment(frames.length);
  return output;
}

function columnName(index) {
  let result = "";
  let value = index + 1;
  while (value > 0) {
    const remainder = (value - 1) % 26;
    result = String.fromCharCode(65 + remainder) + result;
    value = Math.floor((value - 1) / 26);
  }
  return result;
}

function rangeFor(rowCount, colCount) {
  return `A1:${columnName(colCount - 1)}${rowCount}`;
}

function writeMatrix(sheet, matrix, tableName) {
  if (!matrix.length) {
    return;
  }
  const range = sheet.getRange(rangeFor(matrix.length, matrix[0].length));
  range.values = matrix;
  range.format.autofitColumns();
  range.format.autofitRows();
  range.format.borders = { preset: "outside", style: "thin", color: "#B8C2CC" };
  sheet.getRange(`A1:${columnName(matrix[0].length - 1)}1`).format = {
    fill: "#172033",
    font: { bold: true, color: "#FFFFFF" },
  };
  sheet.tables.add(rangeFor(matrix.length, matrix[0].length), true, tableName);
  sheet.freezePanes.freezeRows(1);
  sheet.showGridLines = false;
}

const correctedFrames = enrichedFrames(parseCsv(await fs.readFile(correctedCsvPath, "utf8")));
const previousFrames = enrichedFrames(parseCsv(await fs.readFile(previousCsvPath, "utf8")));
const octaveRows = parseCsv(await fs.readFile(octaveCsvPath, "utf8"));
const correctedSummary = summarize(correctedFrames);
const previousSummary = summarize(previousFrames);
const correctedSegments = segments(correctedFrames);
const previousSegments = segments(previousFrames);

const workbook = Workbook.create();
const summary = workbook.worksheets.add("Summary");
const segmentSheet = workbook.worksheets.add("Segments");
const frameSheet = workbook.worksheets.add("Frames");
const octaveSheet = workbook.worksheets.add("Octave Candidates");

summary.showGridLines = false;
summary.getRange("A1:H1").merge();
summary.getRange("A1").values = [["Cello Scale Test Analysis"]];
summary.getRange("A1").format = {
  fill: "#111827",
  font: { bold: true, color: "#FFFFFF", size: 16 },
};
summary.getRange("A3:B8").values = [
  ["Source WAV", "datasets/cello_scale_test.wav"],
  ["Corrected CSV", "tracking_engine/build/cello_scale_test_engine_frames_low_memory_fix.csv"],
  ["Previous CSV", "tracking_engine/build/cello_scale_test_engine_frames.csv"],
  ["Engine Profile", "Strings"],
  ["Fix Focus", "Low-string octave candidate correction"],
  ["Generated", new Date()],
];
summary.getRange("A3:A8").format = { font: { bold: true }, fill: "#EEF2F7" };
summary.getRange("B8").format.numberFormat = "yyyy-mm-dd hh:mm";

const summaryTable = [
  ["Metric", "Before Fix", "After Fix"],
  ["Frame count", previousSummary.frameCount, correctedSummary.frameCount],
  ["Locked frame count", previousSummary.lockedFrameCount, correctedSummary.lockedFrameCount],
  ["First locked frame", previousSummary.firstLockedFrame, correctedSummary.firstLockedFrame],
  ["Confidence mean", previousSummary.confidenceMean, correctedSummary.confidenceMean],
  ["Confidence min", previousSummary.confidenceMin, correctedSummary.confidenceMin],
  ["Confidence max", previousSummary.confidenceMax, correctedSummary.confidenceMax],
  ["Frequency mean Hz", previousSummary.frequencyMeanHz, correctedSummary.frequencyMeanHz],
];
summary.getRange("A11:C18").values = summaryTable;
summary.getRange("A11:C11").format = {
  fill: "#172033",
  font: { bold: true, color: "#FFFFFF" },
};
summary.getRange("B15:C17").format.numberFormat = "0.000";
summary.getRange("B18:C18").format.numberFormat = "0.000";
summary.getRange("A11:C18").format.borders = {
  preset: "all",
  style: "thin",
  color: "#D6DEE8",
};

const segmentPreview = correctedSegments.map((row) => [
  row.note,
  row.startSeconds,
  row.endSeconds,
  row.frames,
  row.meanHz,
  row.meanConfidence,
]);
summary.getRange("E11:J11").values = [[
  "Corrected Segments",
  "Start s",
  "End s",
  "Frames",
  "Mean Hz",
  "Mean Confidence",
]];
summary.getRange(`E12:J${11 + segmentPreview.length}`).values = segmentPreview;
summary.getRange(`E11:J${11 + segmentPreview.length}`).format.borders = {
  preset: "all",
  style: "thin",
  color: "#D6DEE8",
};
summary.getRange("E11:J11").format = {
  fill: "#172033",
  font: { bold: true, color: "#FFFFFF" },
};
summary.getRange(`F12:G${11 + segmentPreview.length}`).format.numberFormat = "0.000";
summary.getRange(`I12:I${11 + segmentPreview.length}`).format.numberFormat = "0.00";
summary.getRange(`J12:J${11 + segmentPreview.length}`).format.numberFormat = "0.000";
summary.getRange("A:J").format.autofitColumns();

const segmentMatrix = [
  ["Version", "Note", "Start Seconds", "End Seconds", "Frames", "Mean Hz", "Mean Confidence"],
  ...previousSegments.map((row) => [
    "Before Fix",
    row.note,
    row.startSeconds,
    row.endSeconds,
    row.frames,
    row.meanHz,
    row.meanConfidence,
  ]),
  ...correctedSegments.map((row) => [
    "After Fix",
    row.note,
    row.startSeconds,
    row.endSeconds,
    row.frames,
    row.meanHz,
    row.meanConfidence,
  ]),
];
writeMatrix(segmentSheet, segmentMatrix, "SegmentsTable");
segmentSheet.getRange(`C2:D${segmentMatrix.length}`).format.numberFormat = "0.000";
segmentSheet.getRange(`F2:F${segmentMatrix.length}`).format.numberFormat = "0.00";
segmentSheet.getRange(`G2:G${segmentMatrix.length}`).format.numberFormat = "0.000";

const frameMatrix = [
  ["Frame", "Time Seconds", "Frequency Hz", "Target Cents", "Confidence", "Locked", "Note"],
  ...correctedFrames.map((row) => [
    row.frame,
    row.timeSeconds,
    row.frequencyHz,
    row.cents,
    row.confidence,
    row.locked,
    row.note,
  ]),
];
writeMatrix(frameSheet, frameMatrix, "FramesTable");
frameSheet.getRange(`B2:D${frameMatrix.length}`).format.numberFormat = "0.000";
frameSheet.getRange(`E2:E${frameMatrix.length}`).format.numberFormat = "0.000";

const octaveMatrix = [
  [
    "Frame",
    "Start Seconds",
    "Raw YIN Hz",
    "RMS",
    "Peak Amplitude",
    "FFT Peak Hz",
    "Harmonic Ratio",
    "Harmonic Count",
    "Octave /1 Quality",
    "Octave /2 Quality",
    "Octave /3 Quality",
    "Octave /4 Quality",
  ],
  ...octaveRows.map((row) => [
    number(row.frame),
    number(row.start_time_seconds),
    number(row.yin_pitch_hz),
    number(row.rms),
    number(row.peak_amplitude),
    number(row.fft_peak_frequency_hz),
    number(row.harmonic_energy_ratio),
    number(row.harmonic_count),
    number(row.octave_1_quality),
    number(row.octave_2_quality),
    number(row.octave_3_quality),
    number(row.octave_4_quality),
  ]),
];
writeMatrix(octaveSheet, octaveMatrix, "OctaveCandidatesTable");
octaveSheet.getRange(`B2:G${octaveMatrix.length}`).format.numberFormat = "0.000";
octaveSheet.getRange(`I2:L${octaveMatrix.length}`).format.numberFormat = "0.000";

const chartDataStart = correctedSegments.length + 14;
summary.getRange(`E${chartDataStart}:F${chartDataStart}`).values = [["Segment", "Mean Hz"]];
summary.getRange(`E${chartDataStart + 1}:F${chartDataStart + correctedSegments.length}`).values =
  correctedSegments.map((row) => [`${row.note} ${row.startSeconds.toFixed(1)}s`, row.meanHz]);
summary.getRange(`F${chartDataStart + 1}:F${chartDataStart + correctedSegments.length}`).format.numberFormat = "0.00";
const chart = summary.charts.add(
  "line",
  summary.getRange(`E${chartDataStart}:F${chartDataStart + correctedSegments.length}`),
);
chart.title = "Corrected Segment Mean Hz";
chart.hasLegend = false;
chart.xAxis = { axisType: "textAxis", textStyle: { fontSize: 9 } };
chart.yAxis = { numberFormatCode: "0" };
chart.setPosition("A21", "J38");

await fs.mkdir(outputDir, { recursive: true });

const inspect = await workbook.inspect({
  kind: "table",
  range: "Summary!A1:J20",
  include: "values,formulas",
  tableMaxRows: 20,
  tableMaxCols: 10,
});
console.log(inspect.ndjson);

const errors = await workbook.inspect({
  kind: "match",
  searchTerm: "#REF!|#DIV/0!|#VALUE!|#NAME\\?|#N/A",
  options: { useRegex: true, maxResults: 300 },
  summary: "final formula error scan",
});
console.log(errors.ndjson);

const renderRanges = {
  Summary: "A1:J38",
  Segments: "A1:G30",
  Frames: "A1:G50",
  "Octave Candidates": "A1:L50",
};
for (const [sheetName, range] of Object.entries(renderRanges)) {
  const preview = await workbook.render({
    sheetName,
    range,
    scale: 1,
    format: "png",
  });
  await fs.writeFile(
    path.join(outputDir, `${sheetName.replaceAll(" ", "_").toLowerCase()}_preview.png`),
    new Uint8Array(await preview.arrayBuffer()),
  );
}

const output = await SpreadsheetFile.exportXlsx(workbook);
await output.save(outputPath);
console.log(`xlsx=${outputPath}`);
