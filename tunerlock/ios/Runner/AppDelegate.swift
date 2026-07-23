import AVFoundation
import Flutter
import UIKit

@main
@objc class AppDelegate: FlutterAppDelegate, FlutterImplicitEngineDelegate {
  private var microphoneBridge: NativeMicrophoneBridge?

  override func application(
    _ application: UIApplication,
    didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
  ) -> Bool {
    return super.application(application, didFinishLaunchingWithOptions: launchOptions)
  }

  func didInitializeImplicitFlutterEngine(_ engineBridge: FlutterImplicitEngineBridge) {
    GeneratedPluginRegistrant.register(with: engineBridge.pluginRegistry)
    if let registrar = engineBridge.pluginRegistry.registrar(
      forPlugin: "NativeMicrophoneBridge"
    ) {
      microphoneBridge = NativeMicrophoneBridge(messenger: registrar.messenger())
    }
  }
}

private final class NativeMicrophoneBridge: NSObject, FlutterStreamHandler {
  private enum Constants {
    static let controlChannel = "tunerlock/microphone"
    static let frameChannel = "tunerlock/microphone_frames"
    static let frameSize = 2048
    static let hopSize = 1024
    static let preferredSampleRate = 48000.0
  }

  private let audioEngine = AVAudioEngine()
  private let audioQueue = DispatchQueue(label: "tunerlock.microphone.audio")
  private var eventSink: FlutterEventSink?
  private var pendingSamples: [Float] = []
  private var sampleRate = Int(Constants.preferredSampleRate)
  private var isRunning = false

  init(messenger: FlutterBinaryMessenger) {
    super.init()

    let methodChannel = FlutterMethodChannel(
      name: Constants.controlChannel,
      binaryMessenger: messenger
    )
    methodChannel.setMethodCallHandler { [weak self] call, result in
      guard let self = self else {
        result(FlutterError(
          code: "unavailable",
          message: "Microphone bridge is unavailable.",
          details: nil
        ))
        return
      }

      switch call.method {
      case "start":
        self.start(result: result)
      case "stop":
        self.stop()
        result(nil)
      default:
        result(FlutterMethodNotImplemented)
      }
    }

    let eventChannel = FlutterEventChannel(
      name: Constants.frameChannel,
      binaryMessenger: messenger
    )
    eventChannel.setStreamHandler(self)
  }

  func onListen(withArguments arguments: Any?, eventSink events: @escaping FlutterEventSink) -> FlutterError? {
    eventSink = events
    return nil
  }

  func onCancel(withArguments arguments: Any?) -> FlutterError? {
    eventSink = nil
    stop()
    return nil
  }

  private func start(result: @escaping FlutterResult) {
    if isRunning {
      result(nil)
      return
    }

    let session = AVAudioSession.sharedInstance()
    session.requestRecordPermission { [weak self] granted in
      guard let self = self else {
        return
      }

      if !granted {
        DispatchQueue.main.async {
          result(FlutterError(
            code: "permission_denied",
            message: "Microphone permission was denied.",
            details: nil
          ))
        }
        return
      }

      do {
        try self.configureAndStartAudioSession(session)
        DispatchQueue.main.async {
          result(nil)
        }
      } catch {
        self.stop()
        DispatchQueue.main.async {
          result(FlutterError(
            code: "audio_start_failed",
            message: error.localizedDescription,
            details: nil
          ))
        }
      }
    }
  }

  private func configureAndStartAudioSession(_ session: AVAudioSession) throws {
    try session.setCategory(
      .playAndRecord,
      mode: .measurement,
      options: [.allowBluetooth, .defaultToSpeaker]
    )
    try session.setPreferredSampleRate(Constants.preferredSampleRate)
    try session.setPreferredIOBufferDuration(
      Double(Constants.hopSize) / Constants.preferredSampleRate
    )
    try session.setActive(true, options: [])

    let inputNode = audioEngine.inputNode
    let inputFormat = inputNode.outputFormat(forBus: 0)
    sampleRate = Int(inputFormat.sampleRate.rounded())
    pendingSamples.removeAll(keepingCapacity: true)

    inputNode.removeTap(onBus: 0)
    inputNode.installTap(
      onBus: 0,
      bufferSize: AVAudioFrameCount(Constants.hopSize),
      format: inputFormat
    ) { [weak self] buffer, _ in
      self?.audioQueue.async {
        self?.append(buffer: buffer)
      }
    }

    audioEngine.prepare()
    try audioEngine.start()
    isRunning = true
  }

  private func stop() {
    audioQueue.sync {
      pendingSamples.removeAll(keepingCapacity: true)
    }

    if audioEngine.isRunning {
      audioEngine.inputNode.removeTap(onBus: 0)
      audioEngine.stop()
    }

    try? AVAudioSession.sharedInstance().setActive(
      false,
      options: .notifyOthersOnDeactivation
    )
    isRunning = false
  }

  private func append(buffer: AVAudioPCMBuffer) {
    guard let channels = buffer.floatChannelData else {
      return
    }

    let channelCount = max(1, Int(buffer.format.channelCount))
    let frameLength = Int(buffer.frameLength)
    pendingSamples.reserveCapacity(pendingSamples.count + frameLength)

    for frameIndex in 0..<frameLength {
      var sample: Float = 0
      for channelIndex in 0..<channelCount {
        sample += channels[channelIndex][frameIndex]
      }
      pendingSamples.append(sample / Float(channelCount))
    }

    while pendingSamples.count >= Constants.frameSize {
      let frame = Array(pendingSamples.prefix(Constants.frameSize))
      emit(samples: frame)
      pendingSamples.removeFirst(min(Constants.hopSize, pendingSamples.count))
    }
  }

  private func emit(samples: [Float]) {
    guard let eventSink = eventSink else {
      return
    }

    let sampleData = samples.withUnsafeBufferPointer { pointer in
      Data(buffer: pointer)
    }
    let payload: [String: Any] = [
      "samples": FlutterStandardTypedData(float32: sampleData),
      "sampleRate": sampleRate,
    ]

    DispatchQueue.main.async {
      eventSink(payload)
    }
  }
}
