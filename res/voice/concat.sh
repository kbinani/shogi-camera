set -ue

declare -A voices=(
  ["color_black"]=1
  ["color_white"]=2

  ["sq_1_1"]=3
  ["sq_1_2"]=4
  ["sq_1_3"]=5
  ["sq_1_4"]=6
  ["sq_1_5"]=7
  ["sq_1_6"]=8
  ["sq_1_7"]=9
  ["sq_1_8"]=10
  ["sq_1_9"]=11

  ["sq_2_1"]=12
  ["sq_2_2"]=13
  ["sq_2_3"]=14
  ["sq_2_4"]=15
  ["sq_2_5"]=16
  ["sq_2_6"]=17
  ["sq_2_7"]=18
  ["sq_2_8"]=19
  ["sq_2_9"]=20

  ["sq_3_1"]=21
  ["sq_3_2"]=22
  ["sq_3_3"]=23
  ["sq_3_4"]=24
  ["sq_3_5"]=25
  ["sq_3_6"]=26
  ["sq_3_7"]=27
  ["sq_3_8"]=28
  ["sq_3_9"]=29

  ["sq_4_1"]=30
  ["sq_4_2"]=31
  ["sq_4_3"]=32
  ["sq_4_4"]=33
  ["sq_4_5"]=34
  ["sq_4_6"]=35
  ["sq_4_7"]=36
  ["sq_4_8"]=37
  ["sq_4_9"]=38

  ["sq_5_1"]=39
  ["sq_5_2"]=40
  ["sq_5_3"]=41
  ["sq_5_4"]=42
  ["sq_5_5"]=43
  ["sq_5_6"]=44
  ["sq_5_7"]=45
  ["sq_5_8"]=46
  ["sq_5_9"]=47

  ["sq_6_1"]=48
  ["sq_6_2"]=49
  ["sq_6_3"]=50
  ["sq_6_4"]=51
  ["sq_6_5"]=52
  ["sq_6_6"]=53
  ["sq_6_7"]=54
  ["sq_6_8"]=55
  ["sq_6_9"]=56

  ["sq_7_1"]=57
  ["sq_7_2"]=58
  ["sq_7_3"]=59
  ["sq_7_4"]=60
  ["sq_7_5"]=61
  ["sq_7_6"]=62
  ["sq_7_7"]=63
  ["sq_7_8"]=64
  ["sq_7_9"]=65

  ["sq_8_1"]=66
  ["sq_8_2"]=67
  ["sq_8_3"]=68
  ["sq_8_4"]=69
  ["sq_8_5"]=70
  ["sq_8_6"]=71
  ["sq_8_7"]=72
  ["sq_8_8"]=73
  ["sq_8_9"]=74

  ["sq_9_1"]=75
  ["sq_9_2"]=76
  ["sq_9_3"]=77
  ["sq_9_4"]=78
  ["sq_9_5"]=79
  ["sq_9_6"]=80
  ["sq_9_7"]=81
  ["sq_9_8"]=82
  ["sq_9_9"]=83

  # 歩
  ["p_Pawn"]=84
  ["p_Lance"]=85
  ["p_Knight"]=86
  ["p_Silver"]=87
  ["p_Gold"]=88
  ["p_Bishop"]=89
  ["p_Rook"]=90
  ["p_King"]=91
  ["pp_Pawn"]=92
  ["pp_Lance"]=93
  ["pp_Knight"]=94
  ["pp_Silver"]=95
  ["pp_Bishop"]=96
  ["pp_Rook"]=97

  # 同歩
  ["t_Pawn"]=98
  ["t_Lance"]=99
  ["t_Knight"]=100
  ["t_Silver"]=101
  ["t_Gold"]=102
  ["t_Bishop"]=103
  ["t_Rook"]=104
  ["t_King"]=105
  # 同と
  ["tp_Pawn"]=106
  ["tp_Lance"]=107
  ["tp_Knight"]=108
  ["tp_Silver"]=109
  ["tp_Bishop"]=110
  ["tp_Rook"]=111

  ["a_Promote"]=112
  ["a_NoPromote"]=113
  ["a_Drop"]=114
  ["a_Right"]=115
  ["a_Left"]=116
  ["a_Up"]=117
  ["a_Down"]=118
  ["a_Sideway"]=119
  ["a_Nearest"]=120
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
      sqmax=$((cnt + 1))
    fi
  fi
done
echo "  .init(square: (9, 0), time: ${timing[$sqmax]}),"
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
      pmax=$((cnt + 1))
    fi
  fi
  if [[ $voice == pp_* ]]; then
    piece=$(echo "$voice" | cut -f2 -d_)
    time=${timing[$cnt]}
    echo "  .init(piece: PieceType.${piece}.rawValue + PieceStatus.Promoted.rawValue, time: $time),"
    if [[ "$pmax" -lt "$cnt" ]]; then
      pmax=$((cnt + 1))
    fi
  fi
done
echo "  .init(piece: 9999, time: ${timing[$pmax]}),"
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
      pmax=$((cnt + 1))
    fi
  fi
  if [[ $voice == tp_* ]]; then
    piece=$(echo "$voice" | cut -f2 -d_)
    time=${timing[$cnt]}
    echo "  .init(piece: PieceType.${piece}.rawValue + PieceStatus.Promoted.rawValue, time: $time),"
    if [[ "$pmax" -lt "$cnt" ]]; then
      pmax=$((cnt + 1))
    fi
  fi
done
echo "  .init(piece: 9999, time: ${timing[$pmax]}),"
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
      amax=$((cnt + 1))
    fi
  fi
done
echo "  .init(action: 9999, time: ${timing[$amax]}),"
echo "]"
