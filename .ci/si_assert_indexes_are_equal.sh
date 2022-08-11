function assert_indexes_are_equal() {
    # Example:
    #   assert_indexes_are_equal synapses ${direct_spi} ${circuit_spi}

    if ! spatial-index-compare $1 "${2}" "${3}"
    then
        echo "The indexes:"
        echo "  $2"
        echo "  $3"
        echo "differ."
        exit -1
    fi
}
