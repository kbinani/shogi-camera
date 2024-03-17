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

  struct Point: Hashable {
    let x: CGFloat
    let y: CGFloat

    init(_ p: CGPoint) {
      self.x = p.x
      self.y = p.y
    }
  }

  private func Draw(ctx: CGContext, lattice: sci.Lattice, visited: inout Set<Point>) {
    if lattice.content.pointee.index() == 0 {
      let sq = sci.ContourFromLatticeContent(lattice.content.pointee)
      let key = Point(sq.pointee.mean().cgPoint)
      guard !visited.contains(key) else {
        return
      }
      visited.insert(.init(sq.pointee.mean().cgPoint))
      ctx.setFillColor(UIColor.red.cgColor)
      ctx.addPath(sq.pointee.cgPath)
      ctx.fillPath()
    } else {
      let p = sci.PieceContourFromLatticeContent(lattice.content.pointee)
      let key = Point(p.pointee.center().cgPoint)
      guard !visited.contains(key) else {
        return
      }
      visited.insert(key)
      ctx.setFillColor(UIColor.blue.cgColor)
      ctx.addPath(p.pointee.cgPath)
      ctx.fillPath()
    }
    lattice.adjacent.store.forEach { adj in
      let p = adj.lock()
      guard p.__convertToBool() else {
        return
      }
      Draw(ctx: ctx, lattice: p.pointee, visited: &visited)
    }
  }

  private var index: Int? = 0

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
    let length = min(width, height) * 0.5 * 0.8
    let dx = length * cos(CGFloat(status.boardDirection))
    let dy = length * sin(CGFloat(status.boardDirection))
    let p0 = CGPoint(x: cx - dx, y: cy - dy)
    let p1 = CGPoint(x: cx + dx, y: cy + dy)
    ctx.move(to: p0)
    ctx.addLine(to: p1)
    ctx.setLineWidth(3)
    ctx.setStrokeColor(UIColor.white.cgColor)
    ctx.strokePath()

    let cap: CGFloat = 12
    let angle: CGFloat = 30
    let a0 = CGFloat(status.boardDirection) + CGFloat.pi + angle / 180 * CGFloat.pi
    let a1 = CGFloat(status.boardDirection) + CGFloat.pi - angle / 180 * CGFloat.pi
    ctx.move(to: .init(x: p1.x + cap * cos(a0), y: p1.y + cap * sin(a0)))
    ctx.addLine(to: p1)
    ctx.addLine(to: .init(x: p1.x + cap * cos(a1), y: p1.y + cap * sin(a1)))
    ctx.setLineCap(.round)
    ctx.strokePath()

    ctx.setFillColor(UIColor.white.cgColor)
    let r: CGFloat = 6
    ctx.fillEllipse(in: .init(x: p0.x - r, y: p0.y - r, width: 2 * r, height: 2 * r))

    if let preciseOutlinePath = status.preciseOutline.value?.cgPath {
      ctx.addPath(preciseOutlinePath)
      ctx.setLineWidth(3)
      ctx.setStrokeColor(UIColor.green.cgColor)
      ctx.strokePath()
    }
  }
}
