function assert_indexes_are_equal() {
    # Example:
    #   assert_indexes_are_equal ${direct_spi} ${circuit_spi}

    if ! spatial-index-compare "${1}" "${2}"
    then
        echo "The indexes:"
        echo "  ${1}"
        echo "  ${2}"
        echo "differ."
        exit -1
    fi
}
