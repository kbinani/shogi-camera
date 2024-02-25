import AVFoundation
import AudioToolbox
import ShogiCameraInput

class Reader {
  private let engine = AVAudioEngine()
  private let node = AVAudioPlayerNode()
  private var file: AVAudioFile!

  init?() {
    guard let voice = Bundle.main.url(forResource: "voice", withExtension: "wav") else {
      return nil
    }
    do {
      file = try AVAudioFile(forReading: voice)
      let session = AVAudioSession.sharedInstance()
      try session.setCategory(.playback, mode: .default)
      try session.setActive(true)
      engine.attach(node)
      engine.connect(node, to: engine.mainMixerNode, format: file.processingFormat)
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
    let square: (file: Int, rank: Int)
    let time: TimeInterval
  }

  struct Piece {
    let piece: UInt32
    let time: TimeInterval
  }

  enum Action: UInt32 {
    case Promote
    case NoPromote
    case Right
    case Left
    case Up
    case Down
    case Sideway  // 寄る
    case Nearest  // 直ぐ
    case Drop  // 打ち
  }

  struct ActionVoice {
    let action: UInt32
    let time: TimeInterval
  }

  enum Misc: UInt32 {
    case Black
    case White
    case Resign
    case Ready
  }
  struct MiscVoice {
    let misc: UInt32
    let time: TimeInterval
  }

  private let positions: [Position] = [
    .init(square: (8, 0), time: 5.888000),
    .init(square: (8, 1), time: 6.826667),
    .init(square: (8, 2), time: 7.712000),
    .init(square: (8, 3), time: 8.554667),
    .init(square: (8, 4), time: 9.397334),
    .init(square: (8, 5), time: 10.261334),
    .init(square: (8, 6), time: 11.168001),
    .init(square: (8, 7), time: 12.053334),
    .init(square: (8, 8), time: 13.024001),
    .init(square: (7, 0), time: 13.920001),
    .init(square: (7, 1), time: 14.880001),
    .init(square: (7, 2), time: 15.765334),
    .init(square: (7, 3), time: 16.597334),
    .init(square: (7, 4), time: 17.440001),
    .init(square: (7, 5), time: 18.304001),
    .init(square: (7, 6), time: 19.210668),
    .init(square: (7, 7), time: 20.096001),
    .init(square: (7, 8), time: 21.066668),
    .init(square: (6, 0), time: 21.962668),
    .init(square: (6, 1), time: 22.933335),
    .init(square: (6, 2), time: 23.850668),
    .init(square: (6, 3), time: 24.693335),
    .init(square: (6, 4), time: 25.546668),
    .init(square: (6, 5), time: 26.453335),
    .init(square: (6, 6), time: 27.402668),
    .init(square: (6, 7), time: 28.330668),
    .init(square: (6, 8), time: 29.312001),
    .init(square: (5, 0), time: 30.229334),
    .init(square: (5, 1), time: 31.168001),
    .init(square: (5, 2), time: 32.010668),
    .init(square: (5, 3), time: 32.800001),
    .init(square: (5, 4), time: 33.589334),
    .init(square: (5, 5), time: 34.442667),
    .init(square: (5, 6), time: 35.328000),
    .init(square: (5, 7), time: 36.192000),
    .init(square: (5, 8), time: 37.109333),
    .init(square: (4, 0), time: 37.952000),
    .init(square: (4, 1), time: 38.890667),
    .init(square: (4, 2), time: 39.744000),
    .init(square: (4, 3), time: 40.576000),
    .init(square: (4, 4), time: 41.365333),
    .init(square: (4, 5), time: 42.208000),
    .init(square: (4, 6), time: 43.072000),
    .init(square: (4, 7), time: 43.925333),
    .init(square: (4, 8), time: 44.864000),
    .init(square: (3, 0), time: 45.760000),
    .init(square: (3, 1), time: 46.709333),
    .init(square: (3, 2), time: 47.562666),
    .init(square: (3, 3), time: 48.394666),
    .init(square: (3, 4), time: 49.215999),
    .init(square: (3, 5), time: 50.069332),
    .init(square: (3, 6), time: 50.997332),
    .init(square: (3, 7), time: 51.903999),
    .init(square: (3, 8), time: 52.874666),
    .init(square: (2, 0), time: 53.770666),
    .init(square: (2, 1), time: 54.698666),
    .init(square: (2, 2), time: 55.573333),
    .init(square: (2, 3), time: 56.416000),
    .init(square: (2, 4), time: 57.258667),
    .init(square: (2, 5), time: 58.144000),
    .init(square: (2, 6), time: 59.072000),
    .init(square: (2, 7), time: 59.978667),
    .init(square: (2, 8), time: 60.949334),
    .init(square: (1, 0), time: 61.866667),
    .init(square: (1, 1), time: 62.837334),
    .init(square: (1, 2), time: 63.754667),
    .init(square: (1, 3), time: 64.640000),
    .init(square: (1, 4), time: 65.514667),
    .init(square: (1, 5), time: 66.442667),
    .init(square: (1, 6), time: 67.413334),
    .init(square: (1, 7), time: 68.362667),
    .init(square: (1, 8), time: 69.408000),
    .init(square: (0, 0), time: 70.346667),
    .init(square: (0, 1), time: 71.360000),
    .init(square: (0, 2), time: 72.298667),
    .init(square: (0, 3), time: 73.184000),
    .init(square: (0, 4), time: 74.069333),
    .init(square: (0, 5), time: 74.986666),
    .init(square: (0, 6), time: 75.957333),
    .init(square: (0, 7), time: 76.885333),
    .init(square: (0, 8), time: 77.888000),
    .init(square: (Int.max, Int.max), time: 78.816000),
  ]
  private let pieces: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 78.816000),
    .init(piece: PieceType.Lance.rawValue, time: 79.424000),
    .init(piece: PieceType.Knight.rawValue, time: 80.213333),
    .init(piece: PieceType.Silver.rawValue, time: 81.034666),
    .init(piece: PieceType.Gold.rawValue, time: 81.610666),
    .init(piece: PieceType.Bishop.rawValue, time: 82.207999),
    .init(piece: PieceType.Rook.rawValue, time: 82.901332),
    .init(piece: PieceType.King.rawValue, time: 83.605332),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 84.298665),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 85.055998),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 85.973331),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 86.890664),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 87.754664),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 88.373331),
    .init(piece: UInt32.max, time: 89.023998),
  ]
  private let takes: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 89.023998),
    .init(piece: PieceType.Lance.rawValue, time: 89.791998),
    .init(piece: PieceType.Knight.rawValue, time: 90.666665),
    .init(piece: PieceType.Silver.rawValue, time: 91.530665),
    .init(piece: PieceType.Gold.rawValue, time: 92.351998),
    .init(piece: PieceType.Bishop.rawValue, time: 93.205331),
    .init(piece: PieceType.Rook.rawValue, time: 94.090664),
    .init(piece: PieceType.King.rawValue, time: 95.018664),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 95.925331),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 96.682664),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 97.834664),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 98.975997),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 100.074664),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 100.917331),
    .init(piece: UInt32.max, time: 101.749331),
  ]
  private let actions: [ActionVoice] = [
    .init(action: Action.Promote.rawValue, time: 101.749331),
    .init(action: Action.NoPromote.rawValue, time: 102.389331),
    .init(action: Action.Drop.rawValue, time: 103.157331),
    .init(action: Action.Right.rawValue, time: 103.861331),
    .init(action: Action.Left.rawValue, time: 104.533331),
    .init(action: Action.Up.rawValue, time: 105.311998),
    .init(action: Action.Down.rawValue, time: 106.015998),
    .init(action: Action.Sideway.rawValue, time: 106.645331),
    .init(action: Action.Nearest.rawValue, time: 107.253331),
    .init(action: UInt32.max, time: 107.935998),
  ]
  private let miscs: [MiscVoice] = [
    .init(misc: Misc.Black.rawValue, time: 0),
    .init(misc: Misc.White.rawValue, time: 0.821333),
    .init(misc: Misc.Resign.rawValue, time: 1.536000),
    .init(misc: Misc.Ready.rawValue, time: 2.549333),
    .init(misc: UInt32.max, time: 5.888000),
  ]

  func playReady() {
    playMisc(.Ready)
  }

  func playNextMoveReady() {
    // ct-keytone2
    AudioServicesPlaySystemSound(1075)
  }

  private func playMisc(_ misc: Misc) {
    let rate = file.fileFormat.sampleRate

    var startColor: Double?
    var endColor: Double?
    for i in 0..<miscs.count - 1 {
      if miscs[i].misc == misc.rawValue {
        startColor = miscs[i].time
        endColor = miscs[i + 1].time
        break
      }
    }
    if let startColor, let endColor {
      node.scheduleSegment(
        file, startingFrame: .init(startColor * rate),
        frameCount: .init((endColor - startColor) * rate), at: nil)
    }
  }

  func playResign() {
    playMisc(.Resign)
  }

  func play(move: Move, last: Move?) {
    let rate = file.fileFormat.sampleRate
    var offset: TimeInterval = 0

    var startColor: Double?
    var endColor: Double?
    let m = move.color == .Black ? Misc.Black : Misc.White
    for i in 0..<miscs.count - 1 {
      if miscs[i].misc == m.rawValue {
        startColor = miscs[i].time
        endColor = miscs[i + 1].time
        break
      }
    }
    if let startColor, let endColor {
      node.scheduleSegment(
        file, startingFrame: .init(offset + startColor * rate),
        frameCount: .init((endColor - startColor) * rate), at: nil)
      offset += endColor - startColor
    }

    if let last, last.to == move.to {
      var startPiece: TimeInterval?
      var endPiece: TimeInterval?
      let piece =
        move.promote == 1 ? sci.PieceTypeFromPiece(move.piece).rawValue : move.piece
      for i in 0..<self.takes.count - 1 {
        let p = self.takes[i]
        if p.piece == sci.RemoveColorFromPiece(piece) {
          startPiece = p.time
          endPiece = self.takes[i + 1].time
          break
        }
      }
      if let startPiece, let endPiece {
        node.scheduleSegment(
          file, startingFrame: .init(offset + startPiece * rate),
          frameCount: .init((endPiece - startPiece) * rate), at: nil)
        offset += endPiece - startPiece
      } else {
        print("駒読み上げ用ボイスが搭載されていない: ", move.piece)
      }
    } else {
      var startSquare: TimeInterval?
      var endSquare: TimeInterval?
      for i in 0..<self.positions.count - 1 {
        let p = self.positions[i]
        if p.square.file == Int(move.to.file.rawValue)
          && p.square.rank == Int(move.to.rank.rawValue)
        {
          startSquare = p.time
          endSquare = self.positions[i + 1].time
          break
        }
      }
      if let startSquare, let endSquare {
        node.scheduleSegment(
          file, startingFrame: .init(offset + startSquare * rate),
          frameCount: .init((endSquare - startSquare) * rate), at: nil)
        offset += endSquare - startSquare
      } else {
        print("マス目読み上げ用ボイスが搭載されていない: file=", move.to.file.rawValue, "rank=", move.to.rank.rawValue)
      }

      let piece =
        move.promote == 1 ? sci.PieceTypeFromPiece(move.piece).rawValue : move.piece
      var startPiece: TimeInterval?
      var endPiece: TimeInterval?
      for i in 0..<self.pieces.count - 1 {
        let p = self.pieces[i]
        if p.piece == sci.RemoveColorFromPiece(piece) {
          startPiece = p.time
          endPiece = self.pieces[i + 1].time
          break
        }
      }
      if let startPiece, let endPiece {
        node.scheduleSegment(
          file, startingFrame: .init(offset + startPiece * rate),
          frameCount: .init((endPiece - startPiece) * rate), at: nil)
        offset += endPiece - startPiece
      } else {
        print("駒読み上げ用ボイスが搭載されていない: ", move.piece)
      }
    }
    if move.promote != 0 {
      let action: Action = move.promote == 1 ? .Promote : .NoPromote
      var startAction: TimeInterval?
      var endAction: TimeInterval?
      for i in 0..<self.actions.count - 1 {
        let a = self.actions[i]
        if a.action == action.rawValue {
          startAction = a.time
          endAction = self.actions[i + 1].time
        }
      }
      if let startAction, let endAction {
        node.scheduleSegment(
          file, startingFrame: .init(offset + startAction * rate),
          frameCount: .init((endAction - startAction) * rate), at: nil)
        offset += endAction - startAction
      } else {
        print("アクション用のボイスが無い")
      }
    }
    var actions: [Action] = []
    if (move.suffix & sci.SuffixType.Right.rawValue) == sci.SuffixType.Right.rawValue {
      actions.append(.Right)
    }
    if (move.suffix & sci.SuffixType.Left.rawValue) == sci.SuffixType.Left.rawValue {
      actions.append(.Left)
    }
    if (move.suffix & sci.SuffixType.Nearest.rawValue) == sci.SuffixType.Nearest.rawValue {
      actions.append(.Nearest)
    }
    if (move.suffix & sci.SuffixType.Up.rawValue) == sci.SuffixType.Up.rawValue {
      actions.append(.Up)
    }
    if (move.suffix & sci.SuffixType.Down.rawValue) == sci.SuffixType.Down.rawValue {
      actions.append(.Down)
    }
    if (move.suffix & sci.SuffixType.Sideway.rawValue) == sci.SuffixType.Sideway.rawValue {
      actions.append(.Sideway)
    }
    for action in actions {
      var startAction: TimeInterval?
      var endAction: TimeInterval?
      for i in 0..<self.actions.count - 1 {
        let a = self.actions[i]
        if a.action == action.rawValue {
          startAction = a.time
          endAction = self.actions[i + 1].time
          break
        }
      }
      if let startAction, let endAction {
        node.scheduleSegment(
          file, startingFrame: .init(offset + startAction * rate),
          frameCount: .init((endAction - startAction) * rate), at: nil)
        offset += endAction - startAction
      } else {
        print("アクション用のボイスが無い")
      }
    }
  }
}
