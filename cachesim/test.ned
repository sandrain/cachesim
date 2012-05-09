
network Test {
    submodules:
        ioapp: IOApplication;
        disk: StorageDevice;

    connections:
        ioapp.out --> { delay = 0; } --> disk.in
}

