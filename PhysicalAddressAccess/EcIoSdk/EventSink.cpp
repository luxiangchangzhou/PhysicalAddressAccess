// EventSink.cpp
#include "eventsink.h"

ULONG EventSink::AddRef() {
    return InterlockedIncrement(&m_lRef);
}

ULONG EventSink::Release() {
    LONG lRef = InterlockedDecrement(&m_lRef);
    if (lRef == 0)
        delete this;
    return lRef;
}

HRESULT EventSink::QueryInterface(REFIID riid, void** ppv) {
    if (riid == IID_IUnknown || riid == IID_IWbemObjectSink) {
        *ppv = (IWbemObjectSink*)this;
        AddRef();
        return WBEM_S_NO_ERROR;
    } else return E_NOINTERFACE;
}


HRESULT EventSink::Indicate(long lObjectCount,
                            IWbemClassObject** apObjArray) {
    HRESULT hres = S_OK;
    int thid = GetCurrentThreadId();
    for (int i = 0; i < lObjectCount; i++) {

        //Assumes that pObj is defined as a pointer
        // to an IWbemClassObject object.

        VARIANT vt;
        BSTR strClassProp = SysAllocString(L"EventDetail");
        HRESULT hr;
        VariantInit(&vt);
        hr = apObjArray[i]->Get(strClassProp, 0, &vt, 0, 0);
        SysFreeString(strClassProp);

        // check the HRESULT to see if the action succeeded.

        if (SUCCEEDED(hr) && (V_VT(&vt) == VT_ARRAY | VT_I1)) {
            //according to XiaoMi WMI Interface Definition v1.00.29.pdf
            //byte 0 event type,i only care about 0x1
            //byte 1 event function,i only care about 0x27
            //once recieve this event,means EC,eflash log ready
            LONG index = 0;
            int eventType = 0;
            int eventFunc = 0;
            SafeArrayGetElement(vt.parray, &index, &eventType);
            index = 1;
            SafeArrayGetElement(vt.parray, &index, &eventFunc);

            if (eventType == 0x1 && eventFunc == 0x27) {
                callBack();
                //wprintf(L"ec log ready!!!\n");
            } else {
                //wprintf(L"unexpected event ,eventType = %d ,eventFunc = %d \n", eventType, eventFunc);
            }

        } else {
            //wprintf(L"Error in getting specified object\n");
        }
        VariantClear(&vt);

        //printf("HID_EVENT20 Event occurred\n");
    }

    return WBEM_S_NO_ERROR;
}

HRESULT EventSink::SetStatus(
    /* [in] */ LONG lFlags,
    /* [in] */ HRESULT hResult,
    /* [in] */ BSTR strParam,
    /* [in] */ IWbemClassObject __RPC_FAR* pObjParam
) {
    if (lFlags == WBEM_STATUS_COMPLETE) {
        //printf("Call complete. hResult = 0x%X\n", hResult);
    } else if (lFlags == WBEM_STATUS_PROGRESS) {
        //printf("Call in progress.\n");
    }

    return WBEM_S_NO_ERROR;
}    // end of EventSink.cpp