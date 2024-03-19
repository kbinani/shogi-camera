import AVFoundation
import AudioToolbox
import ShogiCamera

class Reader {
  private let engine = AVAudioEngine()
  private let node = AVAudioPlayerNode()
  private var file: AVAudioFile!
  // 読み上げ中のメッセージが読み終わる時刻
  private var latest: Date?

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
    case WrongMoveWarningLeading
    case WrongMoveWarningTrailing
    case YourTurnBlack
    case YourTurnWhite
  }
  struct MiscVoice {
    let misc: UInt32
    let time: TimeInterval
  }

  private let positions: [Position] = [
    .init(square: (8, 0), time: 13.162666),
    .init(square: (8, 1), time: 14.101333),
    .init(square: (8, 2), time: 14.986666),
    .init(square: (8, 3), time: 15.829333),
    .init(square: (8, 4), time: 16.672000),
    .init(square: (8, 5), time: 17.536000),
    .init(square: (8, 6), time: 18.442667),
    .init(square: (8, 7), time: 19.328000),
    .init(square: (8, 8), time: 20.298667),
    .init(square: (7, 0), time: 21.194667),
    .init(square: (7, 1), time: 22.154667),
    .init(square: (7, 2), time: 23.040000),
    .init(square: (7, 3), time: 23.872000),
    .init(square: (7, 4), time: 24.714667),
    .init(square: (7, 5), time: 25.578667),
    .init(square: (7, 6), time: 26.485334),
    .init(square: (7, 7), time: 27.370667),
    .init(square: (7, 8), time: 28.341334),
    .init(square: (6, 0), time: 29.237334),
    .init(square: (6, 1), time: 30.208001),
    .init(square: (6, 2), time: 31.125334),
    .init(square: (6, 3), time: 31.968001),
    .init(square: (6, 4), time: 32.821334),
    .init(square: (6, 5), time: 33.728001),
    .init(square: (6, 6), time: 34.677334),
    .init(square: (6, 7), time: 35.605334),
    .init(square: (6, 8), time: 36.586667),
    .init(square: (5, 0), time: 37.504000),
    .init(square: (5, 1), time: 38.442667),
    .init(square: (5, 2), time: 39.285334),
    .init(square: (5, 3), time: 40.074667),
    .init(square: (5, 4), time: 40.864000),
    .init(square: (5, 5), time: 41.717333),
    .init(square: (5, 6), time: 42.602666),
    .init(square: (5, 7), time: 43.466666),
    .init(square: (5, 8), time: 44.383999),
    .init(square: (4, 0), time: 45.226666),
    .init(square: (4, 1), time: 46.165333),
    .init(square: (4, 2), time: 47.018666),
    .init(square: (4, 3), time: 47.850666),
    .init(square: (4, 4), time: 48.639999),
    .init(square: (4, 5), time: 49.482666),
    .init(square: (4, 6), time: 50.346666),
    .init(square: (4, 7), time: 51.199999),
    .init(square: (4, 8), time: 52.138666),
    .init(square: (3, 0), time: 53.034666),
    .init(square: (3, 1), time: 53.983999),
    .init(square: (3, 2), time: 54.837332),
    .init(square: (3, 3), time: 55.669332),
    .init(square: (3, 4), time: 56.490665),
    .init(square: (3, 5), time: 57.343998),
    .init(square: (3, 6), time: 58.271998),
    .init(square: (3, 7), time: 59.178665),
    .init(square: (3, 8), time: 60.149332),
    .init(square: (2, 0), time: 61.045332),
    .init(square: (2, 1), time: 61.973332),
    .init(square: (2, 2), time: 62.847999),
    .init(square: (2, 3), time: 63.690666),
    .init(square: (2, 4), time: 64.533333),
    .init(square: (2, 5), time: 65.418666),
    .init(square: (2, 6), time: 66.346666),
    .init(square: (2, 7), time: 67.253333),
    .init(square: (2, 8), time: 68.224000),
    .init(square: (1, 0), time: 69.141333),
    .init(square: (1, 1), time: 70.112000),
    .init(square: (1, 2), time: 71.029333),
    .init(square: (1, 3), time: 71.914666),
    .init(square: (1, 4), time: 72.789333),
    .init(square: (1, 5), time: 73.717333),
    .init(square: (1, 6), time: 74.688000),
    .init(square: (1, 7), time: 75.637333),
    .init(square: (1, 8), time: 76.682666),
    .init(square: (0, 0), time: 77.621333),
    .init(square: (0, 1), time: 78.634666),
    .init(square: (0, 2), time: 79.573333),
    .init(square: (0, 3), time: 80.458666),
    .init(square: (0, 4), time: 81.343999),
    .init(square: (0, 5), time: 82.261332),
    .init(square: (0, 6), time: 83.231999),
    .init(square: (0, 7), time: 84.159999),
    .init(square: (0, 8), time: 85.162666),
    .init(square: (Int.max, Int.max), time: 86.090666),
  ]
  private let pieces: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 86.090666),
    .init(piece: PieceType.Lance.rawValue, time: 86.698666),
    .init(piece: PieceType.Knight.rawValue, time: 87.487999),
    .init(piece: PieceType.Silver.rawValue, time: 88.309332),
    .init(piece: PieceType.Gold.rawValue, time: 88.885332),
    .init(piece: PieceType.Bishop.rawValue, time: 89.482665),
    .init(piece: PieceType.Rook.rawValue, time: 90.175998),
    .init(piece: PieceType.King.rawValue, time: 90.879998),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 91.573331),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 92.330664),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 93.247997),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 94.165330),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 95.029330),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 95.647997),
    .init(piece: UInt32.max, time: 96.298664),
  ]
  private let takes: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 96.298664),
    .init(piece: PieceType.Lance.rawValue, time: 97.066664),
    .init(piece: PieceType.Knight.rawValue, time: 97.941331),
    .init(piece: PieceType.Silver.rawValue, time: 98.805331),
    .init(piece: PieceType.Gold.rawValue, time: 99.626664),
    .init(piece: PieceType.Bishop.rawValue, time: 100.479997),
    .init(piece: PieceType.Rook.rawValue, time: 101.365330),
    .init(piece: PieceType.King.rawValue, time: 102.293330),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 103.199997),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 103.957330),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 105.109330),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 106.250663),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 107.349330),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 108.191997),
    .init(piece: UInt32.max, time: 109.023997),
  ]
  private let actions: [ActionVoice] = [
    .init(action: Action.Promote.rawValue, time: 109.023997),
    .init(action: Action.NoPromote.rawValue, time: 109.663997),
    .init(action: Action.Drop.rawValue, time: 110.431997),
    .init(action: Action.Right.rawValue, time: 111.135997),
    .init(action: Action.Left.rawValue, time: 111.807997),
    .init(action: Action.Up.rawValue, time: 112.586664),
    .init(action: Action.Down.rawValue, time: 113.290664),
    .init(action: Action.Sideway.rawValue, time: 113.919997),
    .init(action: Action.Nearest.rawValue, time: 114.527997),
    .init(action: UInt32.max, time: 115.210664),
  ]
  private let miscs: [MiscVoice] = [
    .init(misc: Misc.Black.rawValue, time: 0),
    .init(misc: Misc.White.rawValue, time: 0.821333),
    .init(misc: Misc.Resign.rawValue, time: 1.536000),
    .init(misc: Misc.Ready.rawValue, time: 2.549333),
    .init(misc: Misc.WrongMoveWarningLeading.rawValue, time: 5.888000),
    .init(misc: Misc.WrongMoveWarningTrailing.rawValue, time: 9.632000),
    .init(misc: Misc.YourTurnBlack.rawValue, time: 10.240000),
    .init(misc: Misc.YourTurnWhite.rawValue, time: 11.765333),
    .init(misc: UInt32.max, time: 13.162666),
  ]

  private var availableOffset: TimeInterval {
    if let latest {
      let now = Date.now
      let delta = latest.timeIntervalSince(now)
      return max(0, delta)
    } else {
      return 0
    }
  }

  func playWrongMoveWarning(expected: Move, last: Move?) {
    var offset = schedule(misc: .WrongMoveWarningLeading, offset: availableOffset)
    offset = schedule(move: expected, last: last, offset: offset)
    offset = schedule(misc: .WrongMoveWarningTrailing, offset: offset)
    updateLatestAfter(seconds: offset)
  }

  func playReady() {
    let offset = schedule(misc: .Ready, offset: availableOffset)
    updateLatestAfter(seconds: offset)
  }

  func playYourTurn(_ color: sci.Color) {
    let offset = schedule(misc: color == sci.Color.Black ? .YourTurnBlack : .YourTurnWhite, offset: availableOffset)
    updateLatestAfter(seconds: offset)
  }

  func playNextMoveReady() {
    // ct-keytone2
    AudioServicesPlaySystemSound(1075)
  }

  private func updateLatestAfter(seconds: TimeInterval) {
    latest = Date.now.addingTimeInterval(seconds)
  }

  @discardableResult
  private func schedule(misc: Misc, offset startOffset: TimeInterval) -> TimeInterval {
    let rate = file.fileFormat.sampleRate
    for i in 0..<miscs.count - 1 {
      if miscs[i].misc == misc.rawValue {
        let start = miscs[i].time
        let end = miscs[i + 1].time
        node.scheduleSegment(
          file, startingFrame: .init(startOffset + start * rate),
          frameCount: .init((end - start) * rate), at: nil)
        return startOffset + (end - start)
      }
    }
    return startOffset
  }

  func playResign() {
    let offset = schedule(misc: .Resign, offset: availableOffset)
    updateLatestAfter(seconds: offset)
  }

  @discardableResult
  private func schedule(move: Move, last: Move?, offset startOffset: TimeInterval) -> TimeInterval {
    var offset = startOffset
    let rate = file.fileFormat.sampleRate
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
    return offset
  }

  func play(move: Move, last: Move?) {
    var offset = schedule(
      misc: move.color == .Black ? Misc.Black : Misc.White, offset: availableOffset)
    offset = schedule(move: move, last: last, offset: offset)
    updateLatestAfter(seconds: offset)
  }
}
