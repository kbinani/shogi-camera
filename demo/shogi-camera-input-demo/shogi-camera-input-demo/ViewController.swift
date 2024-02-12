import Foundation
import UIKit
import AVKit
import MetalKit

class ViewController : UIViewController {
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
        view.layer.borderWidth = 1
        view.layer.borderColor = UIColor.red.cgColor
        self.view.addSubview(view)
        self.videoDisplayView = view
    }
    
    private func initCamera() {
        guard let device = AVCaptureDevice.devices().first(where: { $0.position == AVCaptureDevice.Position.back }) else {
            return
        }
        guard let deviceInput = try? AVCaptureDeviceInput(device: device) else {
            return
        }
        let videoDataOutput = AVCaptureVideoDataOutput()
        videoDataOutput.videoSettings = [kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA]
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
        guard let commandQueue, let ciImage, let drawable = videoDisplayView?.currentDrawable, let renderContext else {
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
        
        renderContext.render(centered, to: drawable.texture, commandBuffer: commandBuffer, bounds: .init(origin: .zero, size: drawableSize), colorSpace: CGColorSpaceCreateDeviceRGB())
        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
}

struct Point {
    let x: Int
    let y: Int
    
    init(x: Int, y: Int) {
        self.x = x
        self.y = y
    }
    
    var point: CGPoint {
        return CGPoint(x: x, y: y)
    }
}

struct Shape {
    private(set) var points: [Point] = []
    
    mutating func add(_ point: Point) {
        points.append(point)
    }
}

extension ViewController: AVCaptureVideoDataOutputSampleBufferDelegate {
    private static func FindSquares(_ image: UIImage) -> [Shape] {
        //TODO:
        return []
    }
    
    func captureOutput(_ output: AVCaptureOutput, didOutput sampleBuffer: CMSampleBuffer, from connection: AVCaptureConnection) {
        guard let videoDisplayView else {
            return
        }
        guard let imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer) else {
            return
        }
        var outputImage = CIImage(cvImageBuffer: imageBuffer)
        let uiImage = UIImage(ciImage: outputImage)
        
        let squares = Self.FindSquares(uiImage)
        
        if !squares.isEmpty {
            UIGraphicsBeginImageContext(uiImage.size)
            guard let ctx = UIGraphicsGetCurrentContext() else {
                return
            }
            defer {
                UIGraphicsEndImageContext()
            }
            uiImage.draw(at: .zero)
            
            squares.enumerated().forEach { (it) in
                let i = it.offset
                let points = it.element.points
                
                guard let first = points.first else {
                    return
                }
                
                ctx.saveGState()
                defer {
                    ctx.restoreGState()
                }
                
                let path = CGMutablePath()
                path.move(to: first.point)
                points.dropFirst().forEach({ (p) in
                    path.addLine(to: p.point)
                })
                path.closeSubpath()
                
                let baseAlpha: CGFloat = 0.2
                if (i == 0) {
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
            
            if let created = UIGraphicsGetImageFromCurrentImageContext(), let img = CIImage(image: created) {
                outputImage = img
            }
        }
        self.ciImage = outputImage
    }
}
