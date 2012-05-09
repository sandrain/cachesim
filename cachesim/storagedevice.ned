simple IOApplication {
    parameters:
		int delay;
        string traceFile;
        @display("i=block/app");
    gates:
        output out;
}

simple StorageDevice {
    parameters:
        int blockSize;
        int blockCount;
        double readLatency;
        double writeLatency;
        @display("i=device/disk");
    gates:
        input in;
}

network Test {
    submodules:
        ioapp: IOApplication;
        disk: StorageDevice;

    connections:
        ioapp.out --> { delay = 0; } --> disk.in;
}

