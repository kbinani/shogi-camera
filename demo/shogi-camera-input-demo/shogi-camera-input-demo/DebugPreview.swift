import AVKit
import Foundation
import ShogiCameraInput
import UIKit
import opencv2

class DebugView: UIView {
  private var captureSession: AVCaptureSession? = nil
  private weak var previewLayer: AVCaptureVideoPreviewLayer?
  private weak var overlayLayer: OverlayLayer?
  private weak var imageViewLayer: ImageViewLayer?
  private weak var boardViewLayer: BoardViewLayer?
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

      // 盤面の向きを表示
      let cx = width * 0.5
      let cy = height * 0.5
      let length = max(width, height) * 10
      let dx = length * cos(CGFloat(status.boardDirection))
      let dy = length * sin(CGFloat(status.boardDirection))
      ctx.move(to: .init(x: cx - dx, y: cy - dy))
      ctx.addLine(to: .init(x: cx + dx, y: cy + dy))
      ctx.setLineWidth(3)
      ctx.setStrokeColor(UIColor.white.cgColor)
      ctx.strokePath()

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
      let rect = CGRect(
        x: width * 0.5 - image.size.width * 0.5 * scale,
        y: height * 0.5 - image.size.height * 0.5 * scale, width: image.size.width * scale,
        height: image.size.height * scale)
      do {
        ctx.saveGState()
        defer {
          ctx.restoreGState()
        }
        ctx.translateBy(x: 0, y: height)
        ctx.scaleBy(x: 1, y: -1)
        ctx.draw(cgImage, in: rect)
      }
      let minSim: CGFloat = 0
      let maxSim: CGFloat = status.stableBoardMaxSimilarity
      for y in 0..<9 {
        for x in 0..<9 {
          let pw = rect.width / 9
          let ph = rect.height / 9
          let px = rect.minX + pw * CGFloat(x)
          let py = rect.minY + ph * CGFloat(y)
          let similarity =
            Mirror(reflecting: Mirror(reflecting: status.similarity).children[AnyIndex(x)].value)
            .children[AnyIndex(y)].value as! Double
          let similarityAgainstStableBoard =
            Mirror(
              reflecting: Mirror(reflecting: status.similarityAgainstStableBoard).children[
                AnyIndex(x)
              ].value
            ).children[AnyIndex(y)].value as! Double
          let bar = min((similarity - minSim) / (maxSim - minSim), 1) * ph
          let sbar =
            min((similarityAgainstStableBoard - minSim) / (maxSim - minSim), 1) * ph

          let r = CGRect(x: px, y: py + ph - bar, width: pw * 0.5, height: bar)
          ctx.setFillColor(UIColor.red.withAlphaComponent(0.2).cgColor)
          ctx.fill([r])
          ctx.setStrokeColor(UIColor.red.cgColor)
          ctx.stroke(r)

          let r2 = CGRect(x: px + pw * 0.5, y: py + ph - sbar, width: pw * 0.5, height: sbar)
          ctx.setFillColor(UIColor.blue.withAlphaComponent(0.2).cgColor)
          ctx.fill([r2])
          ctx.setStrokeColor(UIColor.blue.cgColor)
          ctx.stroke(r2)
        }
      }
    }
  }

  class BoardViewLayer: CALayer {
    struct Board {
      let position: sci.Position
      let blackHand: [PieceType]
      let whiteHand: [PieceType]
      let move: Move?
    }

    var board: Board = .init(
      position: sci.MakePosition(.None), blackHand: [], whiteHand: [], move: nil)
    {
      didSet {
        setNeedsDisplay()
      }
    }

    override func draw(in ctx: CGContext) {
      let size = self.bounds.size
      let aspect: CGFloat = 1.1
      let margin: UIEdgeInsets = .init(top: 0, left: 0, bottom: 0, right: 0)
      let w = min(
        (size.height - margin.top - margin.bottom) / 9,
        (size.width - margin.left - margin.right) / 12 / aspect)
      let h = w * aspect
      let cx = size.width * 0.5
      let cy = size.height * 0.5

      // 移動
      if let move = board.move {
        ctx.setFillColor(UIColor.lightGray.cgColor)
        if let from = move.from.value {
          ctx.fill([
            CGRect(
              x: cx + (-4.5 + CGFloat(from.file.rawValue)) * w,
              y: cy + (-4.5 + CGFloat(from.rank.rawValue)) * h, width: w, height: h)
          ])
        }
        ctx.fill([
          CGRect(
            x: cx + (-4.5 + CGFloat(move.to.file.rawValue)) * w,
            y: cy + (-4.5 + CGFloat(move.to.rank.rawValue)) * h, width: w, height: h)
        ])
      }

      // 縦線
      ctx.setStrokeColor(UIColor.gray.cgColor)
      for i in 1..<9 {
        let x = cx + w * (-4.5 + CGFloat(i))
        ctx.beginPath()
        ctx.move(to: .init(x: x, y: cy - 4.5 * h))
        ctx.addLine(to: .init(x: x, y: cy + 4.5 * h))
        ctx.strokePath()
      }
      // 横線
      for i in 1..<9 {
        let y = cy + h * (-4.5 + CGFloat(i))
        ctx.beginPath()
        ctx.move(to: .init(x: cx - w * 4.5, y: y))
        ctx.addLine(to: .init(x: cx + w * 4.5, y: y))
        ctx.strokePath()
      }
      // 枠
      ctx.setStrokeColor(UIColor.black.cgColor)
      ctx.beginPath()
      ctx.addRect(.init(x: cx - w * 4.5, y: cy - h * 4.5, width: w * 9, height: h * 9))
      ctx.strokePath()

      let (r0, r1, r2, r3, r4, r5, r6, r7, r8) = board.position.pieces
      var pieces: [[sci.Piece]] = []
      for r in [r0, r1, r2, r3, r4, r5, r6, r7, r8] {
        let (c0, c1, c2, c3, c4, c5, c6, c7, c8) = r
        let row: [sci.Piece] = [c0, c1, c2, c3, c4, c5, c6, c7, c8]
        pieces.append(row)
      }

      let fontSize = w * 0.8
      let font = CTFont(.application, size: fontSize, language: "ja-JP" as CFString)
      for y in 0..<9 {
        for x in 0..<9 {
          let p = pieces[x][y]
          let color = sci.ColorFromPiece(p)
          if p == 0 {
            continue
          }
          let u8str = sci.ShortStringFromPieceTypeAndStatus(sci.RemoveColorFromPiece(p))
          guard let s = sci.Utility.CFStringFromU8String(u8str)?.takeRetainedValue() else {
            continue
          }
          let sx = cx + (-4.5 + CGFloat(x)) * w
          let sy = cy + (-4.5 + CGFloat(y)) * h
          Self.Draw(
            str: s as String, font: font, ctx: ctx, box: .init(x: sx, y: sy, width: w, height: h),
            rotate: color == sci.Color.White)
        }
      }
      // ポッチ
      let dotRadius = w * 0.1
      ctx.setFillColor(UIColor.gray.cgColor)
      let topLeft = CGRect(
        x: cx - 1.5 * w - dotRadius, y: cy - 1.5 * h - dotRadius, width: dotRadius * 2,
        height: dotRadius * 2)
      ctx.fillEllipse(in: topLeft)
      ctx.fillEllipse(
        in: .init(
          x: topLeft.minX + 3 * w, y: topLeft.minY, width: topLeft.width, height: topLeft.height))
      ctx.fillEllipse(
        in: .init(
          x: topLeft.minX, y: topLeft.minY + 3 * h, width: topLeft.width, height: topLeft.height))
      ctx.fillEllipse(
        in: .init(
          x: topLeft.minX + 3 * w, y: topLeft.minY + 3 * h, width: topLeft.width,
          height: topLeft.height))

      // 持ち駒
      var white: [PieceType: Int] = [:]
      var black: [PieceType: Int] = [:]
      for hand in board.blackHand {
        black[hand] = (black[hand] ?? 0) + 1
      }
      for hand in board.whiteHand {
        white[hand] = (white[hand] ?? 0) + 1
      }
      var by = cy + 3.5 * h
      var wy = cy - 4.5 * h
      for piece in [
        PieceType.Pawn, PieceType.Lance, PieceType.Knight, PieceType.Silver, PieceType.Gold,
        PieceType.Bishop, PieceType.Rook,
      ] {
        let u8str = sci.ShortStringFromPieceTypeAndStatus(piece.rawValue)
        guard let cf = sci.Utility.CFStringFromU8String(u8str)?.takeRetainedValue() else {
          continue
        }
        let s = cf as String
        if let count = black[piece] {
          let ss = count > 1 ? s + String(count) : s
          Self.Draw(
            str: ss, font: font, ctx: ctx, box: .init(x: cx + 4.5 * w, y: by, width: w, height: h),
            rotate: false)
          by -= h
        }
        if let count = white[piece] {
          let ss = count > 1 ? s + String(count) : s
          Self.Draw(
            str: ss, font: font, ctx: ctx, box: .init(x: cx - 5.5 * w, y: wy, width: w, height: h),
            rotate: true)
          wy += h
        }
      }
      Self.Draw(
        str: "☗", font: font, ctx: ctx, box: .init(x: cx + 4.5 * w, y: by, width: w, height: h),
        rotate: false)
      Self.Draw(
        str: "☖", font: font, ctx: ctx, box: .init(x: cx - 5.5 * w, y: wy, width: w, height: h),
        rotate: true)
    }

    private static func Draw(str: String, font: CTFont, ctx: CGContext, box: CGRect, rotate: Bool) {
      let ascent = CTFontGetAscent(font)
      let descent = CTFontGetDescent(font)
      let fontSize = CTFontGetSize(font)
      let str = NSAttributedString(
        string: str, attributes: [.font: font, .foregroundColor: UIColor.black.cgColor])
      let line = CTLineCreateWithAttributedString(str as CFAttributedString)
      ctx.saveGState()
      defer {
        ctx.restoreGState()
      }
      let mtx: CGAffineTransform =
        if rotate {
          CGAffineTransform.identity
            .concatenating(
              .init(translationX: -fontSize * 0.5, y: descent - (ascent + descent) * 0.5)
            )
            .concatenating(.init(scaleX: 1, y: -1))
            .concatenating(.init(rotationAngle: .pi))
            .concatenating(
              .init(
                translationX: box.minX + box.width * 0.5,
                y: box.minY + box.height * 0.5))
        } else {
          CGAffineTransform.identity
            .concatenating(.init(translationX: 0, y: descent - (ascent + descent) * 0.5))
            .concatenating(.init(scaleX: 1, y: -1))
            .concatenating(
              .init(
                translationX: box.minX + box.width * 0.5 - fontSize * 0.5,
                y: box.minY + box.height * 0.5))
        }
      ctx.concatenate(mtx)
      ctx.textMatrix = CGAffineTransform.identity
      CTLineDraw(line, ctx)
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

    let imageViewLayer = ImageViewLayer()
    self.layer.addSublayer(imageViewLayer)
    self.imageViewLayer = imageViewLayer

    let boardViewLayer = BoardViewLayer()
    boardViewLayer.backgroundColor = UIColor.white.cgColor
    boardViewLayer.setNeedsDisplay()
    self.layer.addSublayer(boardViewLayer)
    self.boardViewLayer = boardViewLayer

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

  override func willMove(toWindow w: UIWindow?) {
    if let w {
      boardViewLayer?.contentsScale = w.screen.scale
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    let w = self.bounds.size.width
    let h = self.bounds.size.height
    previewLayer?.frame = .init(x: 0, y: 0, width: w * 0.5, height: h * 0.5)
    overlayLayer?.frame = .init(x: 0, y: 0, width: w * 0.5, height: h * 0.5)
    imageViewLayer?.frame = .init(x: w * 0.5, y: 0, width: w * 0.5, height: h * 0.5)
    boardViewLayer?.frame = .init(x: 0, y: h * 0.5, width: w, height: h * 0.5)
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
    boardViewLayer?.board = .init(
      position: status.game.position, blackHand: .init(status.game.handBlack),
      whiteHand: .init(status.game.handWhite), move: status.game.moves.last)
  }
}
