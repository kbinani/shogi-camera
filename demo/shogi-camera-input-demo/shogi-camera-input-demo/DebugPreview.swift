import AVKit
import Foundation
import MyModule
import UIKit
import opencv2

class DebugView: UIView {
  private var captureSession: AVCaptureSession? = nil
  private weak var previewLayer: AVCaptureVideoPreviewLayer?
  private weak var overlayLayer: OverlayLayer?
  private var session: sci.SessionWrapper?

  class OverlayLayer: CALayer {
    var status: sci.Session.Status? {
      didSet {
        setNeedsDisplay()
      }
    }

    override func draw(in ctx: CGContext) {
      super.draw(in: ctx)
      guard let status else {
        return
      }
      ctx.saveGState()
      defer {
        ctx.restoreGState()
      }
      let size = self.bounds.size
      let width = CGFloat(status.width)
      let height = CGFloat(status.height)
      let scale = min(size.width / height, size.height / width)
      let tx = CGAffineTransform(translationX: -width * 0.5, y: -height * 0.5)
        .concatenating(.init(rotationAngle: CGFloat.pi * 0.5))
        .concatenating(.init(scaleX: scale, y: scale))
        .concatenating(.init(translationX: size.width * 0.5, y: size.height * 0.5))
      ctx.concatenate(tx)
      status.contours.forEach { shape in
        guard let first = shape.points.first else {
          return
        }
        let path = CGMutablePath()
        path.move(to: .init(x: CGFloat(first.x), y: CGFloat(first.y)))
        shape.points.dropFirst().forEach({ (p) in
          path.addLine(to: .init(x: CGFloat(p.x), y: CGFloat(p.y)))
        })
        path.closeSubpath()

        let color: UIColor = shape.points.size() == 4 ? .red : .blue

        ctx.setFillColor(color.withAlphaComponent(0.2).cgColor)
        ctx.addPath(path)
        ctx.fillPath()

        ctx.setStrokeColor(color.cgColor)
        ctx.addPath(path)
        ctx.setLineWidth(3)
        ctx.strokePath()
      }
    }
  }

  init() {
    super.init(frame: .zero)

    guard
      let device = AVCaptureDevice.devices().first(where: {
        $0.position == AVCaptureDevice.Position.back
      })
    else {
      return
    }
    guard let deviceInput = try? AVCaptureDeviceInput(device: device) else {
      return
    }
    let videoDataOutput = AVCaptureVideoDataOutput()
    videoDataOutput.videoSettings = [
      kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA
    ]
    videoDataOutput.setSampleBufferDelegate(self, queue: DispatchQueue.main)
    videoDataOutput.alwaysDiscardsLateVideoFrames = true
    let session = AVCaptureSession()
    guard session.canAddInput(deviceInput) else {
      return
    }
    session.addInput(deviceInput)
    guard session.canAddOutput(videoDataOutput) else {
      return
    }
    session.addOutput(videoDataOutput)

    session.sessionPreset = AVCaptureSession.Preset.medium
    if let range = device.activeFormat.videoSupportedFrameRateRanges.max(by: { left, right in
      left.maxFrameRate < right.maxFrameRate
    }) {
      do {
        try device.lockForConfiguration()
        defer {
          device.unlockForConfiguration()
        }
        let rate = min(max(5, range.minFrameRate), range.maxFrameRate)
        device.activeVideoMinFrameDuration = CMTime(
          seconds: 1 / rate, preferredTimescale: 1_000_000)
      } catch {
        print(error)
      }
    }
    self.captureSession = session
    let preview = AVCaptureVideoPreviewLayer(session: session)
    preview.videoGravity = .resizeAspect
    if #available(iOS 17, *) {
      preview.connection?.videoRotationAngle = 90
    } else {
      preview.connection?.videoOrientation = .portrait
    }
    self.layer.addSublayer(preview)
    self.previewLayer = preview

    let overlay = OverlayLayer()
    self.layer.addSublayer(overlay)
    self.overlayLayer = overlay
    self.session = .init()

    DispatchQueue.global().async { [weak session] in
      session?.startRunning()
    }
  }

  deinit {
    if let captureSession {
      captureSession.stopRunning()
      captureSession.outputs.forEach({ captureSession.removeOutput($0) })
      captureSession.inputs.forEach({ captureSession.removeInput($0) })
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    previewLayer?.frame = .init(origin: .zero, size: self.bounds.size)
    overlayLayer?.frame = .init(origin: .zero, size: self.bounds.size)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}

extension DebugView: AVCaptureVideoDataOutputSampleBufferDelegate {
  private static func Convert(uiImage: UIImage) -> UIImage? {
    UIGraphicsBeginImageContext(uiImage.size)
    defer {
      UIGraphicsEndImageContext()
    }
    uiImage.draw(in: CGRect(origin: .zero, size: uiImage.size))
    return UIGraphicsGetImageFromCurrentImageContext()
  }

  func captureOutput(
    _ output: AVCaptureOutput, didOutput sampleBuffer: CMSampleBuffer,
    from connection: AVCaptureConnection
  ) {
    guard var session else {
      return
    }
    guard let imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer) else {
      return
    }
    let ciImage = CIImage(cvImageBuffer: imageBuffer)
    let uiImage = UIImage(ciImage: ciImage)

    guard let converted = Self.Convert(uiImage: uiImage) else {
      return
    }
    let mat = sci.Session.MatFromUIImage(Unmanaged.passUnretained(converted).toOpaque())
    session.push(mat)
    overlayLayer?.status = session.status()
  }
}
