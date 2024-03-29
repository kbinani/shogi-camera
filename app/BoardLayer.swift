import ShogiCamera
import UIKit

class BoardLayer: CALayer {
  struct Board {
    let position: sci.Position
    let blackHand: [PieceType]
    let whiteHand: [PieceType]
    let move: Move?
    let showArrow: Bool
  }

  var status: sci.Status? {
    didSet {
      guard let status else {
        return
      }
      self.board = .init(
        position: status.game.position,
        blackHand: .init(status.game.handBlack),
        whiteHand: .init(status.game.handWhite),
        move: status.game.moves.last,
        showArrow: status.waitingMove)
    }
  }

  private var board: Board = .init(position: sci.MakePosition(.平手, nil), blackHand: [], whiteHand: [], move: nil, showArrow: false) {
    didSet {
      setNeedsDisplay()
    }
  }

  override func draw(in ctx: CGContext) {
    let size = self.bounds.size
    let aspect: CGFloat = 1.1
    let margin: UIEdgeInsets = .init(top: 0, left: 0, bottom: 0, right: 0)
    let w = min((size.height - margin.top - margin.bottom) / 9, (size.width - margin.left - margin.right) / 12 / aspect)
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
    ctx.setLineWidth(w * 0.03)
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
    ctx.setLineWidth(w * 0.04)
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
    var arrowFrom: CGPoint? = nil
    if let from = board.move?.from.value {
      arrowFrom = CGPoint(
        x: cx + (-4 + CGFloat(from.file.rawValue)) * w,
        y: cy + (-4 + CGFloat(from.rank.rawValue)) * h)
    }
    for piece in [PieceType.Pawn, PieceType.Lance, PieceType.Knight, PieceType.Silver, PieceType.Gold, PieceType.Bishop, PieceType.Rook] {
      let u8str = sci.ShortStringFromPieceTypeAndStatus(piece.rawValue)
      guard let cf = sci.Utility.CFStringFromU8String(u8str)?.takeRetainedValue() else {
        continue
      }
      let s = cf as String
      if let count = black[piece] {
        let ss = count > 1 ? s + String(count) : s
        let box = CGRect(x: cx + 4.5 * w, y: by, width: w, height: h)
        Self.Draw(str: ss, font: font, ctx: ctx, box: box, rotate: false)
        by -= h
        if let move = board.move, board.showArrow, move.color == .Black, !move.from.__convertToBool(), piece == sci.PieceTypeFromPiece(move.piece) {
          arrowFrom = CGPoint(x: box.midX, y: box.midY)
        }
      }
      if let count = white[piece] {
        let ss = count > 1 ? s + String(count) : s
        let box = CGRect(x: cx - 5.5 * w, y: wy, width: w, height: h)
        Self.Draw(str: ss, font: font, ctx: ctx, box: box, rotate: true)
        wy += h
        if let move = board.move, board.showArrow, move.color == .White, !move.from.__convertToBool(), piece == sci.PieceTypeFromPiece(move.piece) {
          arrowFrom = CGPoint(x: box.midX, y: box.midY)
        }
      }
    }
    Self.Draw(
      str: "☗", font: font, ctx: ctx, box: .init(x: cx + 4.5 * w, y: by, width: w, height: h),
      rotate: false)
    Self.Draw(
      str: "☖", font: font, ctx: ctx, box: .init(x: cx - 5.5 * w, y: wy, width: w, height: h),
      rotate: true)

    if let arrowFrom, board.showArrow, let move = board.move {
      ctx.setAlpha(0.7)
      ctx.beginTransparencyLayer(auxiliaryInfo: nil)
      defer {
        ctx.endTransparencyLayer()
      }
      let arrowTo = CGPoint(
        x: cx + (-4 + CGFloat(move.to.file.rawValue)) * w,
        y: cy + (-4 + CGFloat(move.to.rank.rawValue)) * h
      )
      let color = UIColor.red.cgColor
      ctx.setStrokeColor(color)
      ctx.move(to: arrowFrom)
      ctx.addLine(to: arrowTo)
      ctx.setLineWidth(w * 0.1)
      ctx.setLineJoin(.round)
      ctx.setLineCap(.round)
      ctx.strokePath()

      // 矢印の傘の角度(片側)
      let arrowAngle: CGFloat = 10
      // 矢印の傘の長さ
      let arrowLength: CGFloat = w * 0.3
      let dx = arrowTo.x - arrowFrom.x
      let dy = arrowTo.y - arrowFrom.y
      let angle = atan2(dy, dx)
      let tip1 = CGPoint(
        x: arrowTo.x + arrowLength * cos(angle + arrowAngle),
        y: arrowTo.y + arrowLength * sin(angle + arrowAngle))
      let tip2 = CGPoint(
        x: arrowTo.x + arrowLength * cos(angle - arrowAngle),
        y: arrowTo.y + arrowLength * sin(angle - arrowAngle))
      ctx.move(to: tip1)
      ctx.addLine(to: arrowTo)
      ctx.addLine(to: tip2)
      ctx.strokePath()

      let radius = dotRadius * 2
      ctx.setFillColor(color)
      ctx.addEllipse(in: .init(x: arrowFrom.x - radius, y: arrowFrom.y - radius, width: radius * 2, height: radius * 2))
      ctx.fillPath()
    }
  }

  private static func Draw(str: String, font: CTFont, ctx: CGContext, box: CGRect, rotate: Bool) {
    let ascent = CTFontGetAscent(font)
    let descent = CTFontGetDescent(font)
    let fontSize = CTFontGetSize(font)
    let str = NSAttributedString(string: str, attributes: [.font: font, .foregroundColor: UIColor.black.cgColor])
    let line = CTLineCreateWithAttributedString(str as CFAttributedString)
    ctx.saveGState()
    defer {
      ctx.restoreGState()
    }
    // 文字の幅として fontSize を使うと中心からズレる(謎)のでズレを補正するための係数.
    let ratio: CGFloat = 0.04
    let mtx: CGAffineTransform =
      if rotate {
        CGAffineTransform.identity
          .concatenating(.init(translationX: -fontSize * (0.5 - ratio), y: descent - (ascent + descent) * 0.5))
          .concatenating(.init(scaleX: 1, y: -1))
          .concatenating(.init(rotationAngle: .pi))
          .concatenating(.init(translationX: box.minX + box.width * 0.5, y: box.minY + box.height * 0.5))
      } else {
        CGAffineTransform.identity
          .concatenating(.init(translationX: 0, y: descent - (ascent + descent) * 0.5))
          .concatenating(.init(scaleX: 1, y: -1))
          .concatenating(.init(translationX: box.minX + box.width * 0.5 - fontSize * (0.5 - ratio), y: box.minY + box.height * 0.5))
      }
    ctx.concatenate(mtx)
    ctx.textMatrix = CGAffineTransform.identity
    CTLineDraw(line, ctx)
  }
}
