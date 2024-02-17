import AVKit
import Foundation
import ShogiCameraInput
import UIKit
import opencv2

extension sci.Contour {
  var cgPath: CGPath {
    let path = CGMutablePath()
    guard let first = self.points.first else {
      return path
    }
    path.move(to: first.cgPoint)
    self.points.dropFirst().forEach({ (p) in
      path.addLine(to: p.cgPoint)
    })
    path.closeSubpath()
    return path
  }
}

extension sci.PieceContour {
  var cgPath: CGPath {
    let path = CGMutablePath()
    guard let first = self.points.first else {
      return path
    }
    path.move(to: first.cgPoint)
    self.points.dropFirst().forEach({ (p) in
      path.addLine(to: p.cgPoint)
    })
    path.closeSubpath()
    return path
  }
}

extension cv.Point {
  var cgPoint: CGPoint {
    return CGPoint(x: CGFloat(x), y: CGFloat(y))
  }
}

extension cv.Point2f {
  var cgPoint: CGPoint {
    return CGPoint(x: CGFloat(x), y: CGFloat(y))
  }
}

class DebugView: UIView {
  private var captureSession: AVCaptureSession? = nil
  private weak var previewLayer: AVCaptureVideoPreviewLayer?
  private weak var overlayLayer: OverlayLayer?
  private weak var imageView: UIImageView?
  private var session: sci.SessionWrapper?

  class OverlayLayer: CALayer {
    var status: sci.Status? {
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

      status.squares.forEach { square in
        ctx.setStrokeColor(UIColor.cyan.cgColor)
        ctx.addPath(square.pointee.cgPath)
        ctx.setLineWidth(1)
        ctx.strokePath()
      }
      status.pieces.forEach { piece in
        ctx.setStrokeColor(UIColor.cyan.cgColor)
        ctx.addPath(piece.pointee.cgPath)
        ctx.setLineWidth(1)
        ctx.strokePath()
      }

      for y in 0..<9 {
        for x in 0..<9 {
          let p = status.detected[x][y]
          if !p.__convertToBool() {
            continue
          }
          let detected = p.pointee
          if detected.points.size() == 4 {
            let path = detected.cgPath
            ctx.setFillColor(UIColor.red.withAlphaComponent(0.2).cgColor)
            ctx.addPath(path)
            ctx.fillPath()

            ctx.setStrokeColor(UIColor.red.cgColor)
            ctx.addPath(path)
            ctx.setLineWidth(3)
            ctx.strokePath()
          } else {
            let path = detected.cgPath
            ctx.setFillColor(UIColor.blue.withAlphaComponent(0.2).cgColor)
            ctx.addPath(path)
            ctx.fillPath()

            ctx.setStrokeColor(UIColor.blue.cgColor)
            ctx.addPath(path)
            ctx.setLineWidth(3)
            ctx.strokePath()

            if let direction = detected.direction(Float(min(width, height) * 0.1)).value?.cgPoint {
              let mean = detected.mean().cgPoint
              ctx.move(to: mean)
              ctx.addLine(to: .init(x: mean.x + direction.x, y: mean.y + direction.y))
              ctx.setLineWidth(1)
              ctx.strokePath()
            }
          }
        }
      }

      status.hgrids.forEach { grid in
        let vx = CGFloat(grid[0])
        let vy = CGFloat(grid[1])
        let x = CGFloat(grid[2])
        let y = CGFloat(grid[3])
        let scale = width * 2 / sqrt(vx * vx + vy * vy)
        let a = CGPoint(x: x - vx * scale, y: y - vy * scale)
        let b = CGPoint(x: x + vx * scale, y: y + vy * scale)
        ctx.move(to: a)
        ctx.addLine(to: b)
        ctx.setLineWidth(1)
        ctx.setStrokeColor(UIColor.white.cgColor)
        ctx.strokePath()
      }
      status.vgrids.forEach { grid in
        let vx = CGFloat(grid[0])
        let vy = CGFloat(grid[1])
        let x = CGFloat(grid[2])
        let y = CGFloat(grid[3])
        let scale = width * 2 / sqrt(vx * vx + vy * vy)
        let a = CGPoint(x: x - vx * scale, y: y - vy * scale)
        let b = CGPoint(x: x + vx * scale, y: y + vy * scale)
        ctx.move(to: a)
        ctx.addLine(to: b)
        ctx.setLineWidth(1)
        ctx.setStrokeColor(UIColor.black.cgColor)
        ctx.strokePath()
      }

      let outlinePath = status.outline.cgPath
      ctx.addPath(outlinePath)
      ctx.setLineWidth(1)
      ctx.setStrokeColor(UIColor.orange.cgColor)
      ctx.strokePath()

      if let preciseOutlinePath = status.preciseOutline.value?.cgPath {
        ctx.addPath(preciseOutlinePath)
        ctx.setLineWidth(3)
        ctx.setStrokeColor(UIColor.green.cgColor)
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
        let rate = min(max(2, range.minFrameRate), range.maxFrameRate)
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

    let imageView = UIImageView(frame: .zero)
    addSubview(imageView)
    self.imageView = imageView

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
    let w = self.bounds.size.width
    let h = self.bounds.size.height
    // previewLayer?.frame = .init(x: 0, y: 0, width: w, height: h)
    // overlayLayer?.frame = .init(x: 0, y: 0, width: w, height: h)
    previewLayer?.frame = .init(x: 0, y: 0, width: w, height: h * 0.5)
    overlayLayer?.frame = .init(x: 0, y: 0, width: w, height: h * 0.5)
    imageView?.frame = .init(x: 0, y: h * 0.5, width: w, height: h * 0.5)
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
    let mat = sci.Utility.MatFromUIImage(Unmanaged.passUnretained(converted).toOpaque())
    session.push(mat)
    let status = session.status()
    overlayLayer?.status = status
    if let ptr = sci.Utility.UIImageFromMat(status.boardWarped) {
      let boardWarped = Unmanaged<UIImage>.fromOpaque(ptr).takeRetainedValue()
      imageView?.image = boardWarped
    }
  }
}
