import AVFoundation
import ShogiCamera

protocol AnalyzerDelegate: AnyObject {
  func analyzerDidUpdateStatus(_ analyzer: Analyzer)
}

class Analyzer {
  let captureSession: AVCaptureSession
  let dimension: CGSize
  weak var delegate: AnalyzerDelegate?

  private(set) var userColor: sci.Color?
  private(set) var aiLevel: Int?
  private let queue: DispatchQueue

  private var session: sci.SessionWrapper
  private let captureDelegate: CaptureDelegate
  private(set) var status: sci.Status = .init() {
    didSet {
      delegate?.analyzerDidUpdateStatus(self)
    }
  }

  fileprivate class CaptureDelegate: NSObject {
    weak var owner: Analyzer?
    private let ciContext: CIContext = .init()
    private var resigned: Bool = false

    func reset() {
      resigned = false
    }
  }

  init?() {
    guard let device = AVCaptureDevice.devices().first(where: { $0.position == AVCaptureDevice.Position.back }) else {
      return nil
    }
    guard let deviceInput = try? AVCaptureDeviceInput(device: device) else {
      return nil
    }
    let videoDataOutput = AVCaptureVideoDataOutput()
    videoDataOutput.videoSettings = [kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA]
    let captureDelegate = CaptureDelegate()
    self.captureDelegate = captureDelegate
    let queue = DispatchQueue(label: "ShogiCameraAnalyzer")
    videoDataOutput.setSampleBufferDelegate(captureDelegate, queue: queue)
    videoDataOutput.alwaysDiscardsLateVideoFrames = true
    let session = AVCaptureSession()
    guard session.canAddInput(deviceInput) else {
      return nil
    }
    session.addInput(deviceInput)
    guard session.canAddOutput(videoDataOutput) else {
      return nil
    }
    session.addOutput(videoDataOutput)

    session.sessionPreset = AVCaptureSession.Preset.medium
    guard
      let range = device.activeFormat.videoSupportedFrameRateRanges.max(by: { left, right in
        left.maxFrameRate < right.maxFrameRate
      })
    else {
      return nil
    }
    do {
      try device.lockForConfiguration()
      defer {
        device.unlockForConfiguration()
      }
      let rate = min(max(5, range.minFrameRate), range.maxFrameRate)
      device.activeVideoMinFrameDuration = CMTime(seconds: 1 / rate, preferredTimescale: 1_000_000)
      self.captureSession = session
      // videoRotationAngle = 90 で回転しているので幅と高さを交換
      let dimension = device.activeFormat.formatDescription.dimensions
      self.dimension = CGSize(width: CGFloat(dimension.height), height: CGFloat(dimension.width))
      self.session = .init()
      self.queue = queue

      captureDelegate.owner = self
    } catch {
      print(error)
      return nil
    }
  }

  func startGame(userColor: sci.Color, aiLevel: Int) {
    self.userColor = userColor
    self.aiLevel = aiLevel
    self.session.startGame(userColor, Int32(aiLevel))
  }

  func resign() {
    guard let userColor else {
      return
    }
    self.session.resign(userColor)
    let s = self.session.status()
    self.status = s
  }

  func reset() {
    self.delegate = nil
    self.session = .init()
    self.status = .init()
    self.userColor = nil
    self.aiLevel = nil
    self.captureDelegate.reset()
  }
}

extension Analyzer.CaptureDelegate: AVCaptureVideoDataOutputSampleBufferDelegate {
  func captureOutput(_ output: AVCaptureOutput, didOutput sampleBuffer: CMSampleBuffer, from connection: AVCaptureConnection) {
    guard !resigned else {
      return
    }
    guard let imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer) else {
      return
    }
    let ciImage = CIImage(cvImageBuffer: imageBuffer)
    let width = CVPixelBufferGetWidth(imageBuffer)
    let height = CVPixelBufferGetHeight(imageBuffer)
    let rect = CGRect(x: 0, y: 0, width: width, height: height)
    guard let cgImage = ciContext.createCGImage(ciImage, from: rect) else {
      return
    }
    let mat = sci.Utility.MatFromCGImage(Unmanaged.passUnretained(cgImage).toOpaque())
    self.owner?.session.push(mat)
    if let status = self.owner?.session.status() {
      DispatchQueue.main.async { [weak self] in
        self?.owner?.status = status
      }
      if status.blackResign || status.whiteResign {
        self.resigned = true
        self.owner?.captureSession.stopRunning()
      }
    }
  }
}
