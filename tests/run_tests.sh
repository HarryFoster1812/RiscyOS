#!/bin/sh

make

pass_count=0
fail_count=0
total_count=0

for file in build/*; do
  total_count=$((total_count + 1))

  echo "Running $file"
  ./"$file"

  exit_code=$?

  if [ $exit_code -eq 0 ]; then
    pass_count=$((pass_count + 1))
  else
    fail_count=$((fail_count + 1))
  fi
done

echo "Passed: $pass_count"
echo "Failed: $fail_count"
echo "Total: $total_count"
