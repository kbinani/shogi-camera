import AVFoundation
import ShogiCameraInput

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
