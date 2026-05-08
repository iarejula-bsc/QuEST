#!/bin/bash
#SBATCH --time=02:00:00
#SBATCH --exclusive
#SBATCH -N65
#SBATCH -o slurm4dmr.log

# Edit configuration details if needed below
#SBATCH --qos=gp_bsccs
#SBATCH -A bsc85

# Developer debug information
# Level 0 --> Never print
# Level 1 --> Print from process 0
# Level 2 --> Print from all processes
export DMR_DEBUG_LEVEL=1 

# Whether or not to print information for plotting results.
# 0 --> Never print
# 1 --> Print analytics
export DMR_PRINT_ANALYTICS=1

# Dependencies of DMR; these assume that you are using MareNostrum5
module use /apps/GPP/DMR/dmr-modules
module load dlb-for-dmr
module load openpmix-for-dmr
module load prrte-for-dmr
module load openmpi-for-dmr

# These will be used if you have not already set them in your environment
DMR_PATH_DEFAULT=${HOME}/.local
SLURM_ROOT_DEFAULT="/home/bsc/bsc608120/.local/slurm-install"
SLURM_CONFDIR_BASE_DEFAULT="/home/bsc/bsc608120/workspaces/slurm/slurm-confdir-base"


# Name of the Slurm script to launch inside the nested instance of SLURM
EXEC="submit_slurm_malleable.sh"



# Uncomment to let Slurm4DMR run indefinitely (until this job's end time)
#EXEC="sleep infinity"

# Ensure same timezone as main scheduler. Edit as appropriate for your case
export TZ=Europe/Madrid

##################################
# Do not edit after this section #
##################################

export DMR_PATH="${DMR_PATH:-$DMR_PATH_DEFAULT}"
export SLURM_ROOT="${SLURM_ROOT:-$SLURM_ROOT_DEFAULT}"
export SLURM_CONFDIR_BASE="${SLURM_CONFDIR_BASE:-$SLURM_CONFDIR_BASE_DEFAULT}"

if [ ! -d "${DMR_PATH}" ]; then
  echo "DMR_PATH was not set to a valid directory. Terminating."
  exit 1
fi

if [ ! -d "${SLURM_ROOT}" ]; then
  echo "SLURM_ROOT was not set to a valid directory. Terminating."
  exit 1
fi

if [ ! -d "${SLURM_CONFDIR_BASE}" ]; then
  echo "SLURM_CONFDIR_BASE was not set to a valid directory. Terminating."
  exit 1
fi

SLURM_BIN=$SLURM_ROOT/bin
SLURM_SBIN=$SLURM_ROOT/sbin
SLURM_CONF_DIR=$SLURM_ROOT/slurm-confdir
SLURM_CONF_FILE=$SLURM_CONF_DIR/slurm.conf

i=0;
nodes=""
comma=","

rm -rf $SLURM_CONF_DIR # Remove any configuration files from old Slurm runs
cp -r $SLURM_CONFDIR_BASE $SLURM_CONF_DIR # Refresh config files from the base
mv $SLURM_CONF_DIR/slurm.conf.base $SLURM_CONF_FILE
echo >> $SLURM_CONF_FILE # Append a line break
echo "SlurmUser = $USER" >> $SLURM_CONF_FILE
echo "SlurmdUser = $USER" >> $SLURM_CONF_FILE
echo "JobCredentialPrivateKey = $SLURM_CONF_DIR/slurm.key" >> $SLURM_CONF_FILE
echo "JobCredentialPublicCertificate = $SLURM_CONF_DIR/slurm.cert" >> $SLURM_CONF_FILE
echo "StateSaveLocation = $SLURM_ROOT/var" >> $SLURM_CONF_FILE
echo "SlurmdSpoolDir = $SLURM_ROOT/var/slurmd.%n " >> $SLURM_CONF_FILE
echo "SlurmctldPidFile = $SLURM_ROOT/var/slurmctld.pid " >> $SLURM_CONF_FILE
echo "SlurmdPidFile = $SLURM_ROOT/var/slurmd.%n.pid " >> $SLURM_CONF_FILE
echo "SlurmctldLogFile = $SLURM_ROOT/var/slurmctld.log" >> $SLURM_CONF_FILE
echo "SlurmdLogFile = $SLURM_ROOT/var/slurmd.%n.log" >> $SLURM_CONF_FILE
echo "AccountingStorageLoc = $SLURM_ROOT/var/accounting" >> $SLURM_CONF_FILE
echo "JobCompLoc = $SLURM_ROOT/var/job_completions" >> $SLURM_CONF_FILE

rm -rf $SLURM_ROOT/var
mkdir -p $SLURM_ROOT/var
rm -rf $SLURM_ROOT/lock/resize
echo "" > hostfile.txt
echo "" >  $SLURM_ROOT/var/accounting

NODELIST="$(scontrol show hostname $SLURM_JOB_NODELIST | paste -d -s)"
for node in $NODELIST; do
	if [ "$node" == "$(hostname)" ]; then
		echo "ControlMachine=$(hostname)" >> $SLURM_CONF_FILE
	else
		echo $node >> hostfile.txt
		echo "NodeName=$node CPUs=224 CoresPerSocket=56 ThreadsPerCore=2 State=Idle Port=7009" >> $SLURM_CONF_FILE
		nodes=$node$comma$nodes
	fi
done;

echo "PartitionName=slurm4dmr Nodes=$(echo $nodes | sed 's/.$//') Default=YES MaxTime=INFINITE State=UP" >> $SLURM_CONF_FILE

NNODES=$(($SLURM_NNODES-1))
NODELIST="$(scontrol show hostname $SLURM_JOB_NODELIST | paste -d, -s)"

MYSLURM_ROOT=${SLURM_ROOT}
MYSLURM_USER=${USER}
MYSLURM_CONF_DIR=$SLURM_CONF_DIR

# Get system info ====================================================
MYSLURM_MASTER=$(hostname)                     # Master node
MYSLURM_IP=$(hostname -i)                      # Master ip
NODELIST=$(scontrol show hostname | paste -d" " -s)

REMOTE_LIST=(${NODELIST/"${MYSLURM_MASTER}"})  # List of remote nodes (removing master)
_SLURM_SLAVES=${REMOTE_LIST[*]}                # "node1 node2 node3"

MYSLURM_SLAVES=${_SLURM_SLAVES// /,}           # "node1,node2,node3"
MYSLURM_NSLAVES=${#REMOTE_LIST[@]}             # number of slaves

if ((MYSLURM_NSLAVES == 0)); then
	echo "Error: MYSLURM_NSLAVES is zero (are you in the login node?)" >&2
	exit 1
fi

echo "# Generating wrapper: mywrapper.sh"
sed -e "s|@MYSLURM_MASTER@|${MYSLURM_MASTER}|g" \
	-e "s|@MYSLURM_USER@|${MYSLURM_USER}|g" \
	-e "s|@MYSLURM_ROOT@|${MYSLURM_ROOT}|g" \
	-e "s|@MYSLURM_CONF_DIR@|${MYSLURM_CONF_DIR}|g" \
	$SLURM_CONFDIR_BASE/mywrapper.sh.base > mywrapper.sh

chmod a+x mywrapper.sh

###Remove & for interactive use!!!
srun -n $SLURM_NNODES mywrapper.sh &
###

sleep 5

export SLURM_CONF=$SLURM_CONF_FILE
$SLURM_BIN/sinfo

$SLURM_ROOT/bin/sbatch $EXEC &

#$SLURM_BIN/squeue
$SLURM_BIN/sinfo

aux=$( $SLURM_BIN/squeue | wc -l );
while [ $aux -gt 1 ]; do
	aux=$( $SLURM_BIN/squeue | wc -l );
	$SLURM_BIN/squeue --format="%.18i %.9u %.25j %.10T %.8C %.10M %.10l %.6D %R"
	echo "$aux jobs remaining...";
	sleep 10;
done

echo "Finishing...";
$SLURM_BIN/sacct
rm mywrapper.sh hostfile.txt

