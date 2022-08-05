if [[ ! -z "${DATADIR}" ]]
then
    export SI_DATADIR="${DATADIR}/spatial_index"
fi

if [[ -z "${SI_DATADIR}" ]]
then
    echo "SI_DATADIR not set."
    exit -1
fi


