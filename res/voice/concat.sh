set -ue

voices=(
  "m_Black"
  "m_White"
  "m_Resign"
  "m_Ready"
  "m_WrongMoveWarningLeading"
  "m_WrongMoveWarningTrailing"
  "m_YourTurnBlack"
  "m_YourTurnWhite"

  "sq_1_1"
  "sq_1_2"
  "sq_1_3"
  "sq_1_4"
  "sq_1_5"
  "sq_1_6"
  "sq_1_7"
  "sq_1_8"
  "sq_1_9"

  "sq_2_1"
  "sq_2_2"
  "sq_2_3"
  "sq_2_4"
  "sq_2_5"
  "sq_2_6"
  "sq_2_7"
  "sq_2_8"
  "sq_2_9"

  "sq_3_1"
  "sq_3_2"
  "sq_3_3"
  "sq_3_4"
  "sq_3_5"
  "sq_3_6"
  "sq_3_7"
  "sq_3_8"
  "sq_3_9"

  "sq_4_1"
  "sq_4_2"
  "sq_4_3"
  "sq_4_4"
  "sq_4_5"
  "sq_4_6"
  "sq_4_7"
  "sq_4_8"
  "sq_4_9"

  "sq_5_1"
  "sq_5_2"
  "sq_5_3"
  "sq_5_4"
  "sq_5_5"
  "sq_5_6"
  "sq_5_7"
  "sq_5_8"
  "sq_5_9"

  "sq_6_1"
  "sq_6_2"
  "sq_6_3"
  "sq_6_4"
  "sq_6_5"
  "sq_6_6"
  "sq_6_7"
  "sq_6_8"
  "sq_6_9"

  "sq_7_1"
  "sq_7_2"
  "sq_7_3"
  "sq_7_4"
  "sq_7_5"
  "sq_7_6"
  "sq_7_7"
  "sq_7_8"
  "sq_7_9"

  "sq_8_1"
  "sq_8_2"
  "sq_8_3"
  "sq_8_4"
  "sq_8_5"
  "sq_8_6"
  "sq_8_7"
  "sq_8_8"
  "sq_8_9"

  "sq_9_1"
  "sq_9_2"
  "sq_9_3"
  "sq_9_4"
  "sq_9_5"
  "sq_9_6"
  "sq_9_7"
  "sq_9_8"
  "sq_9_9"

  # 歩
  "p_Pawn"
  "p_Lance"
  "p_Knight"
  "p_Silver"
  "p_Gold"
  "p_Bishop"
  "p_Rook"
  "p_King"
  "pp_Pawn"
  "pp_Lance"
  "pp_Knight"
  "pp_Silver"
  "pp_Bishop"
  "pp_Rook"

  # 同歩
  "t_Pawn"
  "t_Lance"
  "t_Knight"
  "t_Silver"
  "t_Gold"
  "t_Bishop"
  "t_Rook"
  "t_King"
  # 同と
  "tp_Pawn"
  "tp_Lance"
  "tp_Knight"
  "tp_Silver"
  "tp_Bishop"
  "tp_Rook"

  "a_Promote"
  "a_NoPromote"
  "a_Drop"
  "a_Right"
  "a_Left"
  "a_Up"
  "a_Down"
  "a_Sideway"
  "a_Nearest"
)

max=1
cnt=0
i=0
for voice in "${voices[@]}"; do
  cnt=$((i + 1))
  if [ "$max" -lt "$cnt" ]; then
    max=$cnt
  fi
  i=$((i + 1))
done
list=$(mktemp)
declare -A durations
i=0
for voice in "${voices[@]}"; do
  cnt=$((i + 1))
  prefix=$(printf "%03d_" $cnt)
  file=$(ls -1 ${prefix}*)
  duration=$(ffprobe "$file" -show_entries format=duration 2>/dev/null | head -2 | tail -1)
  echo "file '$(pwd)/$file'" >> "$list"
  durations["$cnt"]=$(echo "$duration" | sed 's/duration=//g')
  i=$((i + 1))
done
declare -A timing
sec=0
i=0
for voice in "${voices[@]}"; do
  cnt=$((i + 1))
  duration=${durations[$cnt]}
  timing[$cnt]=$sec
  sec=$(echo "$sec + $duration" | bc)
  i=$((i + 1))
done
timing[$((max + 1))]=$sec
ffmpeg -safe 0 -f concat -i "$list" -af volume=8dB voice.wav
rm -f "$list"

echo "private let positions: [Position] = ["
i=0
sqmax=0
for voice in "${voices[@]}"; do
  cnt=$((i + 1))
  if [[ $voice == sq_* ]]; then
    x=$(echo "$voice" | cut -f2 -d_)
    x=$((9 - $x))
    y=$(echo "$voice" | cut -f3 -d_)
    y=$(($y - 1))
    time=${timing[$cnt]}
    echo "  .init(square: ($x, $y), time: $time),"
    if [[ "$sqmax" -lt "$cnt" ]]; then
      sqmax=$cnt
    fi
  fi
  i=$((i + 1))
done
echo "  .init(square: (Int.max, Int.max), time: ${timing[$((sqmax + 1))]}),"
echo "]"

echo "private let pieces: [Piece] = ["
i=0
pmax=0
for voice in "${voices[@]}"; do
  cnt=$((i + 1))
  if [[ $voice == p_* ]]; then
    piece=$(echo "$voice" | cut -f2 -d_)
    time=${timing[$cnt]}
    echo "  .init(piece: PieceType.${piece}.rawValue, time: $time),"
    if [[ "$pmax" -lt "$cnt" ]]; then
      pmax=$cnt
    fi
  fi
  if [[ $voice == pp_* ]]; then
    piece=$(echo "$voice" | cut -f2 -d_)
    time=${timing[$cnt]}
    echo "  .init(piece: PieceType.${piece}.rawValue + PieceStatus.Promoted.rawValue, time: $time),"
    if [[ "$pmax" -lt "$cnt" ]]; then
      pmax=$cnt
    fi
  fi
  i=$((i + 1))
done
echo "  .init(piece: UInt32.max, time: ${timing[$((pmax + 1))]}),"
echo "]"

echo "private let takes: [Piece] = ["
i=0
pmax=0
for voice in "${voices[@]}"; do
  cnt=$((i + 1))
  if [[ $voice == t_* ]]; then
    piece=$(echo "$voice" | cut -f2 -d_)
    time=${timing[$cnt]}
    echo "  .init(piece: PieceType.${piece}.rawValue, time: $time),"
    if [[ "$pmax" -lt "$cnt" ]]; then
      pmax=$cnt
    fi
  fi
  if [[ $voice == tp_* ]]; then
    piece=$(echo "$voice" | cut -f2 -d_)
    time=${timing[$cnt]}
    echo "  .init(piece: PieceType.${piece}.rawValue + PieceStatus.Promoted.rawValue, time: $time),"
    if [[ "$pmax" -lt "$cnt" ]]; then
      pmax=$cnt
    fi
  fi
  i=$((i + 1))
done
echo "  .init(piece: UInt32.max, time: ${timing[$((pmax + 1))]}),"
echo "]"

echo "private let actions: [ActionVoice] = ["
i=0
amax=0
for voice in "${voices[@]}"; do
  cnt=$((i + 1))
  if [[ $voice == a_* ]]; then
    action=$(echo "$voice" | cut -f2 -d_)
    time=${timing[$cnt]}
    echo "  .init(action: Action.${action}.rawValue, time: $time),"
    if [[ "$amax" -lt "$cnt" ]]; then
      amax=$cnt
    fi
  fi
  i=$((i + 1))
done
echo "  .init(action: UInt32.max, time: ${timing[$((amax + 1))]}),"
echo "]"

echo "private let miscs: [MiscVoice] = ["
i=0
mmax=0
for voice in "${voices[@]}"; do
  cnt=$((i + 1))
  if [[ $voice == m_* ]]; then
    misc=$(echo "$voice" | cut -f2 -d_)
    time=${timing[$cnt]}
    echo "  .init(misc: Misc.${misc}.rawValue, time: $time),"
    if [[ "$mmax" -lt "$cnt" ]]; then
      mmax=$cnt
    fi
  fi
  i=$((i + 1))
done
echo "  .init(misc: UInt32.max, time: ${timing[$((mmax + 1))]}),"
echo "]"
