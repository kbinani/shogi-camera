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
    lattice.adjacent.forEach { adj in
      Draw(ctx: ctx, lattice: adj.pointee, visited: &visited)
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

    //    if status.lattices.size() > 0 {
    //      let index: Int = ((self.index ?? 0) + 1) % Int(status.lattices.size())
    //      let lattice = status.lattices[index]
    //      var visited = Set<Point>()
    //      Draw(ctx: ctx, lattice: lattice.pointee, visited: &visited)
    //      self.index = index
    //    }

    status.clusters.first?.forEach { cluster in
      let key = cluster.first
      let x = key.first
      let y = key.second
      var first = true
      cluster.second.forEach { lattice in
        if lattice.pointee.content.pointee.index() == 0 {
          let sq = sci.ContourFromLatticeContent(lattice.pointee.content.pointee)
          ctx.addPath(sq.pointee.cgPath)
          ctx.setStrokeColor(UIColor.red.cgColor)
          ctx.strokePath()
        } else {
          let p = sci.PieceContourFromLatticeContent(lattice.pointee.content.pointee)
          ctx.addPath(p.pointee.cgPath)
          ctx.setStrokeColor(UIColor.blue.cgColor)
          ctx.strokePath()
        }
        if first {
          first = false
          let center = sci.CenterFromLatticeContent(lattice.pointee.content.pointee)
          let s = NSAttributedString(string: "(\(x), \(y))", attributes: [:])
          let line = CTLineCreateWithAttributedString(s)
          ctx.textMatrix = .identity
          ctx.textPosition = center.cgPoint
          CTLineDraw(line, ctx)
        }
      }
    }

    //    for y in 0..<9 {
    //      for x in 0..<9 {
    //        let p = status.detected[x][y]
    //        if !p.__convertToBool() {
    //          continue
    //        }
    //        let detected = p.pointee
    //        if detected.points.size() == 4 {
    //          let path = detected.cgPath
    //          ctx.setFillColor(UIColor.red.withAlphaComponent(0.2).cgColor)
    //          ctx.addPath(path)
    //          ctx.fillPath()
    //
    //          ctx.setStrokeColor(UIColor.red.cgColor)
    //          ctx.addPath(path)
    //          ctx.setLineWidth(3)
    //          ctx.strokePath()
    //        } else {
    //          let path = detected.cgPath
    //          ctx.setFillColor(UIColor.blue.withAlphaComponent(0.2).cgColor)
    //          ctx.addPath(path)
    //          ctx.fillPath()
    //
    //          ctx.setStrokeColor(UIColor.blue.cgColor)
    //          ctx.addPath(path)
    //          ctx.setLineWidth(3)
    //          ctx.strokePath()
    //
    //          if let direction = detected.direction(Float(min(width, height) * 0.1)).value?.cgPoint {
    //            let mean = detected.mean().cgPoint
    //            ctx.move(to: mean)
    //            ctx.addLine(to: .init(x: mean.x + direction.x, y: mean.y + direction.y))
    //            ctx.setLineWidth(1)
    //            ctx.strokePath()
    //          }
    //        }
    //      }
    //    }

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
