#!/bin/sh

# Kiểm tra số lượng tham số đầu vào
if [ "$#" -ne 3 ]; then
  echo "Sử dụng: $0 <chuỗi> <thư mục> <tên tập tin lưu kết quả>"
  exit 1
fi

# Gán các tham số vào biến
string="$1"
folder="$2"
output_file="$3"

# Kiểm tra xem thư mục có tồn tại hay không
if [ ! -d "$folder" ]; then
  echo "Thu muc $folder khong ton tai" >"$output_file"
  exit 1
fi

# Đếm số lần xuất hiện của chuỗi trong các tập tin trong thư mục
count_exist=0

for file in "./$folder"/*; do
  if [ -f "$file" ]; then
    count=$(grep -o "$string" "$file" | wc -l)
    count_exist=$((count_exist + count))
  fi
done

# Ghi kết quả vào tập tin
if [ "$count_exist" -gt 0 ]; then
  echo "Chuoi $string xuat hien $count_exist lan trong thu muc $folder" >"$output_file"
else
  echo "Trong thu muc $folder khong co tap tin nao chua chuoi $string" >"$output_file"
fi
