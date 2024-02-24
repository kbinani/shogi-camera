set -ue

declare -A voices=(
  ["m_Black"]=1
  ["m_White"]=2
  ["m_Resign"]=3

  ["sq_1_1"]=4
  ["sq_1_2"]=5
  ["sq_1_3"]=6
  ["sq_1_4"]=7
  ["sq_1_5"]=8
  ["sq_1_6"]=9
  ["sq_1_7"]=10
  ["sq_1_8"]=11
  ["sq_1_9"]=12

  ["sq_2_1"]=13
  ["sq_2_2"]=14
  ["sq_2_3"]=15
  ["sq_2_4"]=16
  ["sq_2_5"]=17
  ["sq_2_6"]=18
  ["sq_2_7"]=19
  ["sq_2_8"]=20
  ["sq_2_9"]=21

  ["sq_3_1"]=22
  ["sq_3_2"]=23
  ["sq_3_3"]=24
  ["sq_3_4"]=25
  ["sq_3_5"]=26
  ["sq_3_6"]=27
  ["sq_3_7"]=28
  ["sq_3_8"]=29
  ["sq_3_9"]=30

  ["sq_4_1"]=31
  ["sq_4_2"]=32
  ["sq_4_3"]=33
  ["sq_4_4"]=34
  ["sq_4_5"]=35
  ["sq_4_6"]=36
  ["sq_4_7"]=37
  ["sq_4_8"]=38
  ["sq_4_9"]=39

  ["sq_5_1"]=40
  ["sq_5_2"]=41
  ["sq_5_3"]=42
  ["sq_5_4"]=43
  ["sq_5_5"]=44
  ["sq_5_6"]=45
  ["sq_5_7"]=46
  ["sq_5_8"]=47
  ["sq_5_9"]=48

  ["sq_6_1"]=49
  ["sq_6_2"]=50
  ["sq_6_3"]=51
  ["sq_6_4"]=52
  ["sq_6_5"]=53
  ["sq_6_6"]=54
  ["sq_6_7"]=55
  ["sq_6_8"]=56
  ["sq_6_9"]=57

  ["sq_7_1"]=58
  ["sq_7_2"]=59
  ["sq_7_3"]=60
  ["sq_7_4"]=61
  ["sq_7_5"]=62
  ["sq_7_6"]=63
  ["sq_7_7"]=64
  ["sq_7_8"]=65
  ["sq_7_9"]=66

  ["sq_8_1"]=67
  ["sq_8_2"]=68
  ["sq_8_3"]=69
  ["sq_8_4"]=70
  ["sq_8_5"]=71
  ["sq_8_6"]=72
  ["sq_8_7"]=73
  ["sq_8_8"]=74
  ["sq_8_9"]=75

  ["sq_9_1"]=76
  ["sq_9_2"]=77
  ["sq_9_3"]=78
  ["sq_9_4"]=79
  ["sq_9_5"]=80
  ["sq_9_6"]=81
  ["sq_9_7"]=82
  ["sq_9_8"]=83
  ["sq_9_9"]=84

  # 歩
  ["p_Pawn"]=85
  ["p_Lance"]=86
  ["p_Knight"]=87
  ["p_Silver"]=88
  ["p_Gold"]=89
  ["p_Bishop"]=90
  ["p_Rook"]=91
  ["p_King"]=92
  ["pp_Pawn"]=93
  ["pp_Lance"]=94
  ["pp_Knight"]=95
  ["pp_Silver"]=96
  ["pp_Bishop"]=97
  ["pp_Rook"]=98

  # 同歩
  ["t_Pawn"]=99
  ["t_Lance"]=100
  ["t_Knight"]=101
  ["t_Silver"]=102
  ["t_Gold"]=103
  ["t_Bishop"]=104
  ["t_Rook"]=105
  ["t_King"]=106
  # 同と
  ["tp_Pawn"]=107
  ["tp_Lance"]=108
  ["tp_Knight"]=109
  ["tp_Silver"]=110
  ["tp_Bishop"]=111
  ["tp_Rook"]=112

  ["a_Promote"]=113
  ["a_NoPromote"]=114
  ["a_Drop"]=115
  ["a_Right"]=116
  ["a_Left"]=117
  ["a_Up"]=118
  ["a_Down"]=119
  ["a_Sideway"]=120
  ["a_Nearest"]=121
)
declare -A numbers
for voice in "${!voices[@]}"; do
  number=${voices[${voice}]}
  numbers[$number]=$voice
done

max=1
for voice in "${!voices[@]}"; do
  number=${voices[${voice}]}
  if [ "$max" -lt "$number" ]; then
    max=$number
  fi
done
list=$(mktemp)
declare -A durations
cnt=0
for i in $(seq 1 $max); do
  cnt=$((cnt + 1))
  prefix=$(printf "%03d_" $i)
  file=$(ls -1 ${prefix}*)
  duration=$(ffprobe "$file" -show_entries format=duration 2>/dev/null | head -2 | tail -1)
  echo "file '$(pwd)/$file'" >> "$list"
  durations["$cnt"]=$(echo "$duration" | sed 's/duration=//g')
done
declare -A timing
sec=0
cnt=0
for i in $(seq 1 $max); do
  cnt=$((cnt + 1))
  duration=${durations[$cnt]}
  timing[$cnt]=$sec
  sec=$(echo "$sec + $duration" | bc)
done
timing[$((cnt + 1))]=$sec
ffmpeg -safe 0 -f concat -i "$list" -af volume=10dB voice.wav
rm -f "$list"

echo "private let positions: [Position] = ["
cnt=0
sqmax=0
for i in $(seq 1 $max); do
  cnt=$((cnt + 1))
  voice=${numbers[$cnt]}
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
done
echo "  .init(square: (Int.max, Int.max), time: ${timing[$((sqmax + 1))]}),"
echo "]"

echo "private let pieces: [Piece] = ["
cnt=0
pmax=0
for i in $(seq 1 $max); do
  cnt=$((cnt + 1))
  voice=${numbers[$cnt]}
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
done
echo "  .init(piece: UInt32.max, time: ${timing[$((pmax + 1))]}),"
echo "]"

echo "private let takes: [Piece] = ["
cnt=0
pmax=0
for i in $(seq 1 $max); do
  cnt=$((cnt + 1))
  voice=${numbers[$cnt]}
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
done
echo "  .init(piece: UInt32.max, time: ${timing[$((pmax + 1))]}),"
echo "]"

echo "private let actions: [ActionVoice] = ["
cnt=0
amax=0
for i in $(seq 1 $max); do
  cnt=$((cnt + 1))
  voice=${numbers[$cnt]}
  if [[ $voice == a_* ]]; then
    action=$(echo "$voice" | cut -f2 -d_)
    time=${timing[$cnt]}
    echo "  .init(action: Action.${action}.rawValue, time: $time),"
    if [[ "$amax" -lt "$cnt" ]]; then
      amax=$cnt
    fi
  fi
done
echo "  .init(action: UInt32.max, time: ${timing[$((amax + 1))]}),"
echo "]"

echo "private let miscs: [MiscVoice] = ["
cnt=0
mmax=0
for i in $(seq 1 $max); do
  cnt=$((cnt + 1))
  voice=${numbers[$cnt]}
  if [[ $voice == m_* ]]; then
    misc=$(echo "$voice" | cut -f2 -d_)
    time=${timing[$cnt]}
    echo "  .init(misc: Misc.${misc}.rawValue, time: $time),"
    if [[ "$mmax" -lt "$cnt" ]]; then
      mmax=$cnt
    fi
  fi
done
echo "  .init(misc: UInt32.max, time: ${timing[$((mmax + 1))]}),"
echo "]"
