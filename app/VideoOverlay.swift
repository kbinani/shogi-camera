import ShogiCamera
import UIKit

class VideoOverlay: CALayer {
  enum State {
    case waitingStableBoard
    case ready
    case cameraNotAvailable
  }

  var enableDebug: Bool = true {
    didSet {

    }
  }

  private var textLayer: CATextLayer!

  init(state: State) {
    self.state = state
    super.init()
    let textLayer = CATextLayer()
    textLayer.isWrapped = true
    textLayer.alignmentMode = .center
    textLayer.allowsFontSubpixelQuantization = true
    self.textLayer = textLayer
    self.addSublayer(textLayer)
    self.update()
  }

  override init(layer: Any) {
    guard let layer = layer as? Self else {
      fatalError()
    }
    self.state = layer.state
    self.textLayer = layer.textLayer
    super.init(layer: layer)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSublayers() {
    super.layoutSublayers()
    var bounds = CGRect(origin: .zero, size: self.bounds.size)
    bounds.expand(-15, -15)
    self.textLayer.frame = bounds
  }

  var state: State {
    didSet {
      guard self.state != oldValue else {
        return
      }
      self.update()
    }
  }

  override var contentsScale: CGFloat {
    didSet {
      self.textLayer.contentsScale = self.contentsScale
    }
  }

  private func update() {
    switch state {
    case .cameraNotAvailable:
      self.textLayer.string = "カメラを初期化できませんでした"
    case .waitingStableBoard:
      self.textLayer.string = "将棋盤全体が映る位置でカメラを固定してください"
    case .ready:
      self.textLayer.string = "準備ができました"
    }
  }

  var status: sci.Status? {
    didSet {
      setNeedsDisplay()
      switch state {
      case .waitingStableBoard:
        if status?.boardReady == true {
          state = .ready
        }
      case .ready:
        if status?.boardReady != true {
          state = .waitingStableBoard
        }
      case .cameraNotAvailable:
        break
      }
    }
  }

  override func draw(in ctx: CGContext) {
    guard enableDebug, let status else {
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
