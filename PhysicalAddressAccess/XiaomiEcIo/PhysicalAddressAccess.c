#include <ntddk.h>
#include "PhysicalAddressAccess.h"

#define _DRIVER_NAME_ "Xiaomiecio"

typedef struct {
    PDEVICE_OBJECT pDev;
    PDEVICE_OBJECT pLowerDev;
    PDEVICE_OBJECT pPDO;
    UNICODE_STRING devName;
    UNICODE_STRING devSymbolicLink;
} DEVICE_EXTENSION, * PDEVICE_EXTENSION;



DRIVER_DISPATCH DispatchCreate;
DRIVER_DISPATCH DispatchClose;
DRIVER_DISPATCH DispatchDeviceControl;

NTSTATUS PhysicalAddressRead(LONGLONG PhysicalAddress, ULONG ReadCount, void* ReadDst);
NTSTATUS PhysicalAddressWrite(LONGLONG PhysicalAddress, ULONG WriteCount, void* WriteSrc);

VOID DriverUnload(struct _DRIVER_OBJECT* DriverObject);


NTSTATUS DriverEntry(struct _DRIVER_OBJECT* DriverObject, PUNICODE_STRING  RegistryPath) {

    //KdBreakPoint();
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT pDev;
    PDEVICE_EXTENSION pDeviceEx;
    UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\PhysicalAddressAccess");


    KdPrint(("PhysicalAddressAccess DriverEntry called------------------------\n"));

    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;

    ntStatus = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), &devName, PHYSICAL_ADDRESS_ACCESS_DEVICE, 0, FALSE, &pDev);
    if (!NT_SUCCESS(ntStatus)) {
        KdPrint(("PhysicalAddressAccess IoCreateDevice called failed------------------------\n"));
        return ntStatus;
    }

    pDev->Flags |= DO_BUFFERED_IO;


    pDeviceEx = pDev->DeviceExtension;
    pDeviceEx->pDev = pDev;
    pDeviceEx->pPDO = NULL;
    pDeviceEx->pLowerDev = NULL;
    RtlInitUnicodeString(&pDeviceEx->devName, L"\\Device\\PhysicalAddressAccess");
    RtlInitUnicodeString(&pDeviceEx->devSymbolicLink, L"\\??\\PhysicalAddressAccess-D2DEBE83-AA54-4DFC-AA7E-2160938BEB88");

    ntStatus = IoCreateSymbolicLink(&pDeviceEx->devSymbolicLink, &devName);
    if (!NT_SUCCESS(ntStatus)) {
        IoDeleteDevice(pDev);
        return ntStatus;

    }

    return ntStatus;
}



NTSTATUS PhysicalAddressRead(LONGLONG PhysicalAddress, ULONG ReadCount, void* ReadDst) {
    //KdPrint(("PhysicalAddressAccess PhysicalAddressRead called------------------------\n"));

    UNICODE_STRING    physicalMemoryDeviceName;          // Name of the section object
    OBJECT_ATTRIBUTES objectAttributes;                  // Description for the section object
    HANDLE            hPhysicalMemoryHandle;             // Handle to the section object
    PHYSICAL_ADDRESS  mappedLength;                      // Length of the frame buffer
    PHYSICAL_ADDRESS  mappedBase;                        // Base physical address (CPU-relative) of the frame buffer
    NTSTATUS ntStatus = STATUS_SUCCESS;
    INT memoryOffset;

    // Allocate a variable to receive the base virtual address of the view.
    // Set it to NULL for input to ZwMapViewOfSection, to specify that the memory
    // manager (rather than the caller) should determine the base virtual address.
    PVOID pViewBase = NULL;

    mappedLength.QuadPart = PAGE_SIZE * 2;
    mappedBase.QuadPart = PhysicalAddress;

    //get the offset
    memoryOffset = mappedBase.QuadPart % 0x1000;

    if (ReadCount > 0x1000 || ReadCount == 0) {
        return STATUS_ABANDONED;
    }


    RtlInitUnicodeString(&physicalMemoryDeviceName, L"\\Device\\PhysicalMemory");

    InitializeObjectAttributes(
        &objectAttributes,
        &physicalMemoryDeviceName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        (HANDLE)NULL,
        (PSECURITY_DESCRIPTOR)NULL);

    // Open a handle to the physical-memory section object.
    ntStatus = ZwOpenSection(&hPhysicalMemoryHandle, SECTION_ALL_ACCESS, &objectAttributes);

    if (NT_SUCCESS(ntStatus)) {
        ntStatus = ZwMapViewOfSection(
                       hPhysicalMemoryHandle,
                       NtCurrentProcess(),
                       &pViewBase,
                       0L,
                       (ULONG_PTR)mappedLength.QuadPart,
                       &mappedBase,
                       (PULONG_PTR)(&(mappedLength.QuadPart)),
                       ViewUnmap,
                       0,
                       PAGE_READWRITE | PAGE_WRITECOMBINE);

        if (NT_SUCCESS(ntStatus)) {
            // pViewBase holds the base virtual address of the view.
            memcpy(ReadDst, (char*)pViewBase + memoryOffset, ReadCount);

            ZwUnmapViewOfSection(NtCurrentProcess(), pViewBase);
        } else {
            KdPrint(("ZwMapViewOfSection ntStatus = %d------------------------\n", ntStatus));
        }

        // Close the handle to the physical-memory section object.
        ZwClose(hPhysicalMemoryHandle);
    } else {
        KdPrint(("ZwOpenSection ntStatus = %d------------------------\n", ntStatus));
    }
    return ntStatus;
}




NTSTATUS PhysicalAddressWrite(LONGLONG PhysicalAddress, ULONG WriteCount, void* WriteSrc) {
    //KdPrint(("PhysicalAddressAccess PhysicalAddressWrite called------------------------\n"));

    UNICODE_STRING    physicalMemoryDeviceName;          // Name of the section object
    OBJECT_ATTRIBUTES objectAttributes;                  // Description for the section object
    HANDLE            hPhysicalMemoryHandle;             // Handle to the section object
    PHYSICAL_ADDRESS  mappedLength;                      // Length of the frame buffer
    PHYSICAL_ADDRESS  mappedBase;                        // Base physical address (CPU-relative) of the frame buffer
    NTSTATUS ntStatus = STATUS_SUCCESS;
    INT memoryOffset;

    // Allocate a variable to receive the base virtual address of the view.
    // Set it to NULL for input to ZwMapViewOfSection, to specify that the memory
    // manager (rather than the caller) should determine the base virtual address.
    PVOID pViewBase = NULL;

    mappedLength.QuadPart = PAGE_SIZE * 2;
    mappedBase.QuadPart = PhysicalAddress;

    //get the offset
    memoryOffset = mappedBase.QuadPart % 0x1000;

    if (WriteCount > 0x1000 || WriteCount == 0) {
        return STATUS_ABANDONED;
    }


    RtlInitUnicodeString(&physicalMemoryDeviceName, L"\\Device\\PhysicalMemory");

    InitializeObjectAttributes(
        &objectAttributes,
        &physicalMemoryDeviceName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        (HANDLE)NULL,
        (PSECURITY_DESCRIPTOR)NULL);

    // Open a handle to the physical-memory section object.
    ntStatus = ZwOpenSection(&hPhysicalMemoryHandle, SECTION_ALL_ACCESS, &objectAttributes);

    if (NT_SUCCESS(ntStatus)) {
        ntStatus = ZwMapViewOfSection(
                       hPhysicalMemoryHandle,
                       NtCurrentProcess(),
                       &pViewBase,
                       0L,
                       (ULONG_PTR)mappedLength.QuadPart,
                       &mappedBase,
                       (PULONG_PTR)(&(mappedLength.QuadPart)),
                       ViewUnmap,
                       0,
                       PAGE_READWRITE | PAGE_WRITECOMBINE);

        if (NT_SUCCESS(ntStatus)) {
            // pViewBase holds the base virtual address of the view.
            memcpy((char*)pViewBase + memoryOffset, WriteSrc, WriteCount);

            ZwUnmapViewOfSection(NtCurrentProcess(), pViewBase);
        } else {
            KdPrint(("ZwMapViewOfSection ntStatus = %d------------------------\n", ntStatus));
        }

        // Close the handle to the physical-memory section object.
        ZwClose(hPhysicalMemoryHandle);
    } else {
        KdPrint(("ZwOpenSection ntStatus = %d------------------------\n", ntStatus));
    }
    return ntStatus;
}





NTSTATUS DispatchDeviceControl(struct _DEVICE_OBJECT* DeviceObject, struct _IRP* Irp) {
    //KdBreakPoint();
    UNREFERENCED_PARAMETER(DeviceObject);
    PIO_STACK_LOCATION pStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG inputBufferLength = pStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outputBufferLength = pStack->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG operationByteCount = 0;

    //io reuse Irp->AssociatedIrp.SystemBuffer

    switch (pStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_PHYSICALADDRESS_READ: {
        if (inputBufferLength != sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST)) {
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
        } else {
            PPHYSICAL_ADDRESS_ACCESS_REQUEST pReadRequest = Irp->AssociatedIrp.SystemBuffer;
            operationByteCount = (pReadRequest->accessCount < outputBufferLength) ? pReadRequest->accessCount : outputBufferLength;
            ntStatus = PhysicalAddressRead(pReadRequest->physicalAddress.QuadPart,
                                           operationByteCount,
                                           Irp->AssociatedIrp.SystemBuffer);
        }

    }
    break;
    case IOCTL_PHYSICALADDRESS_WRITE: {
        if (inputBufferLength != sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST)) {
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
        } else {
            PPHYSICAL_ADDRESS_ACCESS_REQUEST pWriteRequest = Irp->AssociatedIrp.SystemBuffer;
            operationByteCount = (pWriteRequest->accessCount < outputBufferLength) ? pWriteRequest->accessCount : outputBufferLength;

            UCHAR* bufToWrite = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
            if (bufToWrite == NULL) {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            ntStatus = PhysicalAddressWrite(pWriteRequest->physicalAddress.QuadPart,
                                            operationByteCount,
                                            bufToWrite);
        }

    }
    break;
    default:
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = ntStatus;
    if (ntStatus == STATUS_SUCCESS) {
        Irp->IoStatus.Information = operationByteCount;
    } else {
        Irp->IoStatus.Information = 0;
    }
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return ntStatus;
}

NTSTATUS DispatchCreate(struct _DEVICE_OBJECT* DeviceObject, struct _IRP* Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    KdPrint(("PhysicalAddressAccess DispatchCreate called------------------------\n"));
    return STATUS_SUCCESS;
}
NTSTATUS DispatchClose(struct _DEVICE_OBJECT* DeviceObject, struct _IRP* Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    KdPrint(("PhysicalAddressAccess DispatchClose called------------------------\n"));
    return STATUS_SUCCESS;
}



VOID DriverUnload(struct _DRIVER_OBJECT* DriverObject) {
    PDEVICE_OBJECT pFDO = NULL;
    pFDO = DriverObject->DeviceObject;
    if (DriverObject->DeviceObject == NULL)
        return;
    PDEVICE_EXTENSION pDeviceEx = pFDO->DeviceExtension;

    while (pFDO) {
        IoDeleteSymbolicLink(&pDeviceEx->devSymbolicLink);
        IoDeleteDevice(pFDO);
        pFDO = pFDO->NextDevice;
        if (!pFDO)
            break;
        pDeviceEx = pFDO->DeviceExtension;
    }

    KdPrint(("PhysicalAddressAccess DriverUnload called------------------------\n"));

}



