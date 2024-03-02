import ShogiCamera
import UIKit

class VideoOverlay: CALayer {
  var enableDebug: Bool = true {
    didSet {

    }
  }

  override init() {
    super.init()
  }

  override init(layer: Any) {
    guard let layer = layer as? Self else {
      fatalError()
    }
    self.enableDebug = layer.enableDebug
    super.init(layer: layer)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  var status: sci.Status? {
    didSet {
      setNeedsDisplay()
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
