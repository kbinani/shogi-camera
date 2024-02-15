import AVKit
import Foundation
import MetalKit
import UIKit
import MyModule
import opencv2

class ViewController: UIViewController {
  private weak var videoDisplayView: MTKView? = nil
  private var renderContext: CIContext? = nil
  private var session: AVCaptureSession? = nil
  private var device: MTLDevice?
  private var commandQueue: MTLCommandQueue?
  private var ciImage: CIImage?

  override func viewWillAppear(_ animated: Bool) {
    initDisplay()
    initCamera()
  }

  override func viewDidDisappear(_ animated: Bool) {
    guard let session = self.session else {
      return
    }
    session.stopRunning()
    session.outputs.forEach({ session.removeOutput($0) })
    session.inputs.forEach({ session.removeInput($0) })
    self.session = nil
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    videoDisplayView?.frame = .init(origin: .zero, size: self.view.bounds.size)
  }

  private func initDisplay() {
    guard let device = MTLCreateSystemDefaultDevice() else {
      return
    }
    guard let commandQueue = device.makeCommandQueue() else {
      return
    }
    self.commandQueue = commandQueue

    let context = CIContext(mtlDevice: device)
    self.renderContext = context

    let view = MTKView(frame: self.view.bounds, device: device)
    view.device = device
    view.delegate = self
    view.framebufferOnly = false
    self.view.addSubview(view)
    self.videoDisplayView = view
  }

  private func initCamera() {
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
    if let max = device.activeFormat.videoSupportedFrameRateRanges.max(by: { left, right in
      left.maxFrameRate < right.maxFrameRate
    }) {
      do {
        try device.lockForConfiguration()
        defer {
          device.unlockForConfiguration()
        }
        device.activeVideoMinFrameDuration = max.maxFrameDuration
      } catch {
        print(error)
      }
    }
    self.session = session

    DispatchQueue.global().async { [weak session] in
      session?.startRunning()
    }
  }
}

extension ViewController: MTKViewDelegate {
  func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {

  }

  func draw(in view: MTKView) {
    guard let commandQueue, let ciImage, let drawable = videoDisplayView?.currentDrawable,
      let renderContext
    else {
      return
    }
    guard let commandBuffer = commandQueue.makeCommandBuffer() else {
      return
    }
    let drawableSize = drawable.layer.drawableSize
    let sx = drawableSize.width / ciImage.extent.width
    let sy = drawableSize.height / ciImage.extent.height
    let scale = min(sx, sy)
    let scaled = ciImage.transformed(by: CGAffineTransform(scaleX: scale, y: scale))

    let tx = max(drawableSize.width - scaled.extent.size.width, 0) / 2
    let ty = max(drawableSize.height - scaled.extent.size.height, 0) / 2
    let centered = scaled.transformed(by: .init(translationX: tx, y: ty))

    renderContext.render(
      centered, to: drawable.texture, commandBuffer: commandBuffer,
      bounds: .init(origin: .zero, size: drawableSize), colorSpace: CGColorSpaceCreateDeviceRGB())
    commandBuffer.present(drawable)
    commandBuffer.commit()
  }
}

extension ViewController: AVCaptureVideoDataOutputSampleBufferDelegate {
  private static func FindSquares(_ input: UIImage, buffer: inout [sci.Shape]) -> UIImage? {
    let mat = sci.Foo.MatFromUIImage(Unmanaged.passUnretained(input).toOpaque())
    let status = sci.Session.FindSquares(mat)
    status.shapes.forEach { shape in
      buffer.append(shape)
    }
    return Unmanaged<UIImage>.fromOpaque(sci.Foo.UIImageFromMat(status.processed)).takeRetainedValue()
  }

  private func convert(uiImage: UIImage) -> UIImage? {
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
    guard let imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer) else {
      return
    }
    var outputImage = CIImage(cvImageBuffer: imageBuffer)
    let tmp = UIImage(ciImage: outputImage)

    guard let uiImage = convert(uiImage: tmp) else {
      return
    }

    var squares: [sci.Shape] = []
    guard let recog = Self.FindSquares(uiImage, buffer: &squares) else {
      return
    }
    guard let gray = convert(uiImage: recog) else {
      return
    }

    if squares.isEmpty {
      if let img = CIImage(image: gray) {
        outputImage = img
      }
    } else {
      UIGraphicsBeginImageContext(uiImage.size)
      guard let ctx = UIGraphicsGetCurrentContext() else {
        return
      }
      defer {
        UIGraphicsEndImageContext()
      }
      print("gray.size=", gray.size, "tmp.size=", tmp.size)
      gray.draw(in: .init(origin: .zero, size: uiImage.size))

      squares.enumerated().forEach { (it) in
        let i = it.offset
        guard let first = it.element.points.first else {
          return
        }

        ctx.saveGState()
        defer {
          ctx.restoreGState()
        }

        let path = CGMutablePath()
        path.move(to: .init(x: CGFloat(first.x), y: CGFloat(first.y)))
        it.element.points.dropFirst().forEach({ (p) in
          path.addLine(to: .init(x: CGFloat(p.x), y: CGFloat(p.y)))
        })
        path.closeSubpath()

        let baseAlpha: CGFloat = 0.2
        if i == 0 {
          let r: CGFloat = 1
          let g: CGFloat = 0
          let b: CGFloat = 0

          ctx.setFillColor(red: r, green: g, blue: b, alpha: baseAlpha)
          ctx.addPath(path)
          ctx.fillPath()

          ctx.setStrokeColor(red: r, green: g, blue: b, alpha: 1)
          ctx.addPath(path)
          ctx.setLineWidth(3)
          ctx.strokePath()
        } else {
          let scale: CGFloat = CGFloat(squares.count - 1) / CGFloat(squares.count)

          ctx.setFillColor(red: 0, green: 0, blue: 1, alpha: baseAlpha * scale)
          ctx.addPath(path)
          ctx.fillPath()

          ctx.setStrokeColor(red: 0, green: 0, blue: 1, alpha: scale)
          ctx.addPath(path)
          ctx.setLineWidth(3)
          ctx.strokePath()
        }
      }

      if let created = UIGraphicsGetImageFromCurrentImageContext(),
        let img = CIImage(image: created)
      {
        outputImage = img
      }
    }
    self.ciImage = outputImage
  }
}
