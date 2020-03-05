# Function to check whether input param is on list of shell scripts
# $1 - <string> absolute path to file
is_it_script () {
  [ $# -eq 0 ] && return 1
  
  readarray list_of_scripts < ./.ci/script-list.txt
  echo "${list_of_scripts[@]}" | grep --silent "$1" && return 0
}

