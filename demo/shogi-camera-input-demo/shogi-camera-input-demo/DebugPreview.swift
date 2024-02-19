import AVKit
import Foundation
import ShogiCameraInput
import UIKit
import opencv2

class Reader {
  private let engine = AVAudioEngine()
  private let node = AVAudioPlayerNode()
  private var buffer: AVAudioPCMBuffer!
  private var file: AVAudioFile!

  init?() {
    guard let voice = Bundle.main.url(forResource: "voice", withExtension: "wav") else {
      return nil
    }
    do {
      let file = try AVAudioFile(forReading: voice)
      guard
        let buffer = AVAudioPCMBuffer(
          pcmFormat: file.processingFormat, frameCapacity: AVAudioFrameCount(file.length))
      else {
        return
      }
      self.file = file
      try file.read(into: buffer)
      self.buffer = buffer

      let session = AVAudioSession.sharedInstance()
      try session.setCategory(.playAndRecord)
      try session.setActive(true)
      engine.attach(node)
      engine.connect(node, to: engine.mainMixerNode, format: file.processingFormat)
      node.volume = 24
      try engine.start()
      node.play()
    } catch {
      print(error)
      return nil
    }
  }

  deinit {
    node.stop()
    engine.stop()
  }

  struct Position {
    let squre: (file: Int, rank: Int)
    let time: TimeInterval
  }
  private let positions: [Position] = [
    .init(squre: (0, 0), time: 1.648),
    .init(squre: (0, 1), time: 2.751),
    .init(squre: (0, 2), time: 3.907),
    .init(squre: (0, 3), time: 5.063),
    .init(squre: (0, 4), time: 6.285),
    .init(squre: (0, 5), time: 7.335),
    .init(squre: (0, 6), time: 8.438),
    .init(squre: (0, 7), time: 9.501),
    .init(squre: (0, 8), time: 10.683),

    .init(squre: (1, 0), time: 11.560),
    .init(squre: (1, 1), time: 12.690),
    .init(squre: (1, 2), time: 13.700),
    .init(squre: (1, 3), time: 14.670),
    .init(squre: (1, 4), time: 15.693),
    .init(squre: (1, 5), time: 16.596),
    .init(squre: (1, 6), time: 17.580),
    .init(squre: (1, 7), time: 17.563),
    .init(squre: (1, 8), time: 19.626),

    .init(squre: (2, 0), time: 20.476),
    .init(squre: (2, 1), time: 21.579),
    .init(squre: (2, 2), time: 22.682),
    .init(squre: (2, 3), time: 23.652),
    .init(squre: (2, 4), time: 24.728),
    .init(squre: (2, 5), time: 25.672),
    .init(squre: (2, 6), time: 26.721),
    .init(squre: (2, 7), time: 27.758),
    .init(squre: (2, 8), time: 28.874),

    .init(squre: (3, 0), time: 29.818),
    .init(squre: (3, 1), time: 30.967),
    .init(squre: (3, 2), time: 31.904),
    .init(squre: (3, 3), time: 32.860),
    .init(squre: (3, 4), time: 33.897),
    .init(squre: (3, 5), time: 34.787),
    .init(squre: (3, 6), time: 35.784),
    .init(squre: (3, 7), time: 36.754),
    .init(squre: (3, 8), time: 37.830),

    .init(squre: (4, 0), time: 38.680),
    .init(squre: (4, 1), time: 39.690),
    .init(squre: (4, 2), time: 40.660),
    .init(squre: (4, 3), time: 41.551),
    .init(squre: (4, 4), time: 42.521),
    .init(squre: (4, 5), time: 43.384),
    .init(squre: (4, 6), time: 44.274),
    .init(squre: (4, 7), time: 45.178),
    .init(squre: (4, 8), time: 46.175),

    .init(squre: (5, 0), time: 47.025),
    .init(squre: (5, 1), time: 48.101),
    .init(squre: (5, 2), time: 49.124),
    .init(squre: (5, 3), time: 50.081),
    .init(squre: (5, 4), time: 51.104),
    .init(squre: (5, 5), time: 51.995),
    .init(squre: (5, 6), time: 52.965),
    .init(squre: (5, 7), time: 53.895),
    .init(squre: (5, 8), time: 54.905),

    .init(squre: (6, 0), time: 55.821),
    .init(squre: (6, 1), time: 56.884),
    .init(squre: (6, 2), time: 57.947),
    .init(squre: (6, 3), time: 58.944),
    .init(squre: (6, 4), time: 59.980),
    .init(squre: (6, 5), time: 60.964),
    .init(squre: (6, 6), time: 61.974),
    .init(squre: (6, 7), time: 62.957),
    .init(squre: (6, 8), time: 64.007),

    .init(squre: (7, 0), time: 64.897),
    .init(squre: (7, 1), time: 66.026),
    .init(squre: (7, 2), time: 67.089),
    .init(squre: (7, 3), time: 68.113),
    .init(squre: (7, 4), time: 69.176),
    .init(squre: (7, 5), time: 70.172),
    .init(squre: (7, 6), time: 71.209),
    .init(squre: (7, 7), time: 72.245),
    .init(squre: (7, 8), time: 73.321),

    .init(squre: (8, 0), time: 74.331),
    .init(squre: (8, 1), time: 75.474),
    .init(squre: (8, 2), time: 76.577),
    .init(squre: (8, 3), time: 77.560),
    .init(squre: (8, 4), time: 78.636),
    .init(squre: (8, 5), time: 79.593),
    .init(squre: (8, 6), time: 80.643),
    .init(squre: (8, 7), time: 81.666),
    .init(squre: (8, 8), time: 82.756),

    .init(squre: (9, 0), time: 83.712),
  ]

  struct Piece {
    let piece: UInt32
    let time: TimeInterval
  }
  private let pieces: [Piece] = [
    .init(piece: sci.PieceType.Pawn.rawValue, time: 83.739),
    .init(piece: sci.PieceType.Lance.rawValue, time: 84.310),
    .init(piece: sci.PieceType.Knight.rawValue, time: 85.094),
    .init(piece: sci.PieceType.Silver.rawValue, time: 85.918),
    .init(piece: sci.PieceType.Gold.rawValue, time: 86.542),
    .init(piece: sci.PieceType.Bishop.rawValue, time: 87.061),
    .init(piece: sci.PieceType.Rook.rawValue, time: 87.738),
    .init(piece: sci.PieceType.King.rawValue, time: 88.456),
    .init(piece: sci.PieceType.Pawn.rawValue + sci.PieceStatus.Promoted.rawValue, time: 89.173),
    .init(piece: sci.PieceType.Lance.rawValue + sci.PieceStatus.Promoted.rawValue, time: 89.904),
    .init(piece: sci.PieceType.Knight.rawValue + sci.PieceStatus.Promoted.rawValue, time: 90.834),
    .init(piece: sci.PieceType.Silver.rawValue + sci.PieceStatus.Promoted.rawValue, time: 91.725),
    .init(piece: sci.PieceType.Bishop.rawValue + sci.PieceStatus.Promoted.rawValue, time: 92.602),
    .init(piece: sci.PieceType.Rook.rawValue + sci.PieceStatus.Promoted.rawValue, time: 93.266),
    .init(piece: 9999, time: 93.266),
  ]

  func play(move: sci.Move) {
    let startColor: Double
    let endColor: Double
    if move.color == .Black {
      startColor = 0
      endColor = 0.864
    } else {
      startColor = 0.864
      endColor = 1.648
    }
    let rate = file.fileFormat.sampleRate
    node.scheduleSegment(
      file, startingFrame: .init(startColor * rate),
      frameCount: .init((endColor - startColor) * rate), at: nil)
    var offset = endColor - startColor

    var startSquare = self.positions[0].time
    var endSquare = self.positions[1].time
    for i in 0..<self.positions.count - 1 {
      let p = self.positions[i]
      if p.squre.file == Int(move.to.file.rawValue) && p.squre.rank == Int(move.to.rank.rawValue) {
        startSquare = p.time
        endSquare = self.positions[i + 1].time
        break
      }
    }
    node.scheduleSegment(
      file, startingFrame: .init(offset + startSquare * rate),
      frameCount: .init((endSquare - startSquare) * rate), at: nil)
    offset += endSquare - startSquare

    var startPiece = self.pieces[0].time
    var endPiece = self.pieces[1].time
    for i in 0..<self.pieces.count - 1 {
      let p = self.pieces[i]
      if p.piece == move.piece {
        startPiece = p.time
        endPiece = self.pieces[i + 1].time
        break
      }
    }
    node.scheduleSegment(
      file, startingFrame: .init(offset + startPiece * rate),
      frameCount: .init((endPiece - startPiece) * rate), at: nil)
    offset += endPiece - startPiece
  }
}

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
  private weak var imageViewLayer: ImageViewLayer?
  private var session: sci.SessionWrapper?
  private let reader: Reader?
  private var moveIndex: Int?

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

  class ImageViewLayer: CALayer {
    var status: sci.Status? {
      didSet {
        setNeedsDisplay()
      }
    }

    override func draw(in ctx: CGContext) {
      guard let status else {
        return
      }
      guard let board = status.stableBoard.value else {
        return
      }
      guard let ptr = sci.Utility.UIImageFromMat(board) else {
        return
      }
      let image = Unmanaged<UIImage>.fromOpaque(ptr).takeRetainedValue()
      guard let cgImage = image.cgImage else {
        return
      }
      ctx.saveGState()
      defer {
        ctx.restoreGState()
      }
      let width = self.bounds.width
      let height = self.bounds.height
      let scale = min(width / image.size.width, height / image.size.height)
      ctx.translateBy(x: 0, y: height)
      ctx.scaleBy(x: 1, y: -1)
      let rect = CGRect(
        x: width * 0.5 - image.size.width * 0.5 * scale,
        y: height * 0.5 - image.size.height * 0.5 * scale, width: image.size.width * scale,
        height: image.size.height * scale)
      ctx.draw(cgImage, in: rect)
      var minSim: CGFloat = CGFloat.infinity
      var maxSim: CGFloat = -CGFloat.infinity
      for y in 0..<9 {
        for x in 0..<9 {
          minSim = min(minSim, status.similarity[x][y])
          maxSim = max(maxSim, status.similarity[x][y])
        }
      }
      minSim = 0
      maxSim = status.stableBoardMaxSimilarity
      for y in 0..<9 {
        for x in 0..<9 {
          let pw = rect.width / 9
          let ph = rect.height / 9
          let px = rect.minX + pw * CGFloat(x)
          let py = rect.minY + ph * CGFloat(y)
          let bar = min((status.similarity[x][y] - minSim) / (maxSim - minSim), 1) * ph
          let barAgainstStable =
            min((status.similarityAgainstStableBoard[x][y] - minSim) / (maxSim - minSim), 1) * ph

          let r = CGRect(x: px, y: py + ph - bar, width: pw * 0.5, height: bar)
          ctx.setFillColor(UIColor.red.withAlphaComponent(0.2).cgColor)
          ctx.fill([r])
          ctx.setStrokeColor(UIColor.red.cgColor)
          ctx.stroke(r)

          let r2 = CGRect(
            x: px + pw * 0.5, y: py + ph - bar, width: pw * 0.5, height: barAgainstStable)
          ctx.setFillColor(UIColor.blue.withAlphaComponent(0.2).cgColor)
          ctx.fill([r2])
          ctx.setStrokeColor(UIColor.blue.cgColor)
          ctx.stroke(r2)
        }
      }
    }
  }

  init() {
    self.reader = Reader()
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

    let imageViewLayer = ImageViewLayer()
    self.layer.addSublayer(imageViewLayer)
    self.imageViewLayer = imageViewLayer

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
    imageViewLayer?.frame = .init(x: 0, y: h * 0.5, width: w, height: h * 0.5)
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
    imageViewLayer?.status = status
    if !status.game.moves.empty() {
      if let moveIndex {
        if moveIndex + 1 < status.game.moves.size() {
          let mv = status.game.moves[moveIndex + 1]
          reader?.play(move: mv)
          self.moveIndex = moveIndex + 1
        }
      } else {
        let mv = status.game.moves[0]
        reader?.play(move: mv)
        moveIndex = 0
      }
    }
  }
}
