#!/bin/sh

# get date
clean_year=`date --date='2 months ago' +%Y`
clean_month=`date --date='2 months ago' +%m`
clean_day=`date --date='2 months ago' +%d`

# define h264 path
h264_path="/usr/local/freeswitch/recordings/video_h264"
cd $h264_path

# ----- clean folder fuction -----
function CleanFolder()
{
  path=$1
  echo "remove path:$path"
  rm -rf $path
}

function CleanDayFolder()
{
  # enter month's folder
  month_folder=$1
  cd $month_folder

  # look through day's folder
  for day_folder in `ls`
  do
    curr_path=`pwd`
    day_path="$curr_path/$day_folder"
    if [ -d "$day_path" ]; then
      ((clean_day_temp=10#$clean_day))
      ((day_folder_temp=10#$day_folder))
      if [ $day_folder_temp -lt $clean_day_temp ]; then
        # remove old day's folder
#        echo "remove clean_day:$clean_day, day_folder:$day_folder, path:$day_path"
        CleanFolder $day_path
      fi
    fi
  done

  # leave month's folder
  cd ..
}

function CleanMonthFolder()
{
  # enter year's folder
  year_folder=$1
  cd $year_folder

  # look through month's folder
  for month_folder in `ls`
  do
    curr_path=`pwd`
    month_path="$curr_path/$month_folder"
    if [ -d "$month_path" ]; then
      ((clean_month_tmp=10#$clean_month))
      ((month_folder_tmp=10#$month_folder))
      if [ $month_folder_tmp -lt $clean_month_tmp ]; then
        # remove old month's folder
#        echo "remove clean_month:$clean_month, month_folder:$month_folder, path:$month_path"
        CleanFolder $month_path
      elif [ $clean_month_tmp -eq $month_folder_tmp ]; then
        # clean day's folder
        CleanDayFolder $month_folder
      fi
    fi
  done

  # leave year's folder
  cd ..
}

function CleanYearFolder()
{
  # look through year's folder
  for year_folder in `ls`
  do
    curr_path=`pwd`
    year_path="$curr_path/$year_folder"
    if [ -d "$year_path" ]; then
      ((clean_year_tmp=10#$clean_year))
      ((year_folder_tmp=10#$year_folder))
      if [ $year_folder_tmp -lt $clean_year_tmp ]; then
        # remove old year folder
#        echo "remove clean_year:$clean_year, year_folder:$year_folder, path:$year_path"
        CleanFolder $year_path
      elif [ $clean_year_tmp -eq $year_folder_tmp ]; then
        # clean month folder
        CleanMonthFolder $year_folder
      fi
    fi 
  done
}
# ---- clean folder function finish ----

# clean folder
CleanYearFolder
