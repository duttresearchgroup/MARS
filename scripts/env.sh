# Extracts the script folder from the current script
__SOURCE="${BASH_SOURCE[0]}"
while [ -h "$__SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  __DIR="$( cd -P "$( dirname "$__SOURCE" )" && pwd )"
  __SOURCE="$(readlink "$__SOURCE")"
  [[ $__SOURCE != /* ]] && __SOURCE="$__DIR/$__SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
export SPARTA_SCRIPTDIR="$( cd -P "$( dirname "$__SOURCE" )" && pwd )"
echo $SPARTA_SCRIPTDIR

# The root dir
export SPARTA_ROOT=$(readlink -f $SPARTA_SCRIPTDIR/..)
echo $SPARTA_ROOT
