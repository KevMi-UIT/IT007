#!/bin/sh

# Kiểm tra số lượng tham số đầu vào
if [ "$#" -ne 4 ]; then
  echo "Sử dụng: $0 <giờ> <phút> <số giờ làm việc> <tên tập tin lưu kết quả>"
  exit 1
fi

# Gán các tham số vào biến
start_hour="$1"
start_minute="$2"
num_trips="$3"
output_file="$4"

# Kiểm tra tính hợp lệ của giờ, phút
if [ "$start_hour" -ge 24 ]; then
  echo "Gio khong hop le" >"$output_file"
  exit 1
fi

if [ "$start_minute" -ge 60 ]; then
  echo "Phut khong hop le" >"$output_file"
  exit 1
fi

# Định nghĩa thời gian cho mỗi chuyến xe và thời gian nghỉ giữa các chuyến
trip_time=110 # Thời gian một chuyến là 110 phút
rest_time=30  # Thời gian nghỉ giữa mỗi chuyến là 30 phút

# Tính tổng thời gian làm việc
total_work_time=$((trip_time * num_trips + rest_time * (num_trips - 1)))

# Kiểm tra nếu tổng thời gian làm việc vượt quá 600 phút (10 giờ)
if [ "$total_work_time" -gt 600 ]; then
  echo "Tong thoi gian lam viec khong hop le" >"$output_file"
  exit 1
fi

# Tính giờ và phút kết thúc
end_minute=$((start_minute + total_work_time))
end_hour=$((start_hour + end_minute / 60))
end_minute=$((end_minute % 60))
end_hour=$((end_hour % 24))

# Lưu kết quả vào tập tin
echo "$end_hour $end_minute" >"$output_file"
