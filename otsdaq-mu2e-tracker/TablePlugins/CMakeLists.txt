include(otsdaq::table)

cet_build_plugin(SubsystemTrackerParametersTable otsdaq::table
    LIBRARIES REG
    otsdaq::SlowControlsTableBase
    otsdaq::XDAQContextTable
    Offline::DataProducts
)

install_headers()
install_source()
