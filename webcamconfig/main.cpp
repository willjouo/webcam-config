#include <iostream>
#include <afxwin.h>
#include <windows.h>
#include <dshow.h>

#pragma comment(lib, "strmiids")

// https://stackoverflow.com/questions/4286223/how-to-get-a-list-of-video-capture-devices-web-cameras-on-windows-c
// https://docs.microsoft.com/en-us/windows/desktop/directshow/find-an-interface-on-a-filter-or-pin
// https://docs.microsoft.com/fr-fr/windows/desktop/DirectShow/displaying-a-filters-property-pages
// https://github.com/obsproject/libdshowcapture/tree/master/source

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
	// Name, path and filter
	std::wstring name;
	std::wstring path;
	IBaseFilter *pFilter = nullptr;

	// Initializes the COM library
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if(FAILED(hr)){
		return 1;
	}

	// Create the System Device Enumerator.
	ICreateDevEnum *pDevEnum;
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));
	if(FAILED(hr)){
		return 1;
	}

	// Create an enumerator for the category.
	IEnumMoniker *pEnum;
	hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
	if(hr == S_FALSE){
		return 1;
	}
	pDevEnum->Release();
	if(FAILED(hr)){
		return 1;
	}

	// Go through devices
	IMoniker *pMoniker = NULL;
	while(pEnum->Next(1, &pMoniker, NULL) == S_OK)
	{
		// Get properties
		IPropertyBag *pPropBag;
		hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
		if(FAILED(hr)){
			pMoniker->Release();
			continue;
		}

		VARIANT var;
		VariantInit(&var);

		// Get description or friendly name.
		hr = pPropBag->Read(L"Description", &var, 0);
		if(FAILED(hr)){
			hr = pPropBag->Read(L"FriendlyName", &var, 0);
		}
		if(SUCCEEDED(hr)){
			printf("%S\n", var.bstrVal);
			name = var.bstrVal;
			VariantClear(&var);
		}

		hr = pPropBag->Read(L"DevicePath", &var, 0);
		if(SUCCEEDED(hr)){
			// The device path is not intended for display.
			printf("Device path: %S\n", var.bstrVal);
			path = var.bstrVal;
			VariantClear(&var);
		}

		// Bind to a filter
		hr = pMoniker->BindToObject(NULL, 0, IID_IBaseFilter, (void**)&pFilter);
		if (SUCCEEDED(hr)){
			printf("Binded\n");
		}

		// Release
		pPropBag->Release();
		pMoniker->Release();
	}
	pEnum->Release();
	
	// Open property dialog
	ISpecifyPropertyPages *pProp;
	hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pProp);
	if(SUCCEEDED(hr)){
		printf("Showing...\n");

		// Get the filter's name and IUnknown pointer.
		FILTER_INFO FilterInfo;
		hr = pFilter->QueryFilterInfo(&FilterInfo);
		IUnknown *pFilterUnk;
		pFilter->QueryInterface(IID_IUnknown, (void **)&pFilterUnk);

		// Show the page. 
		CAUUID caGUID;
		pProp->GetPages(&caGUID);
		pProp->Release();
		OleCreatePropertyFrame(
			nullptr,                // Parent window
			0, 0,                   // Reserved
			FilterInfo.achName,     // Caption for the dialog box
			1,                      // Number of objects (just the filter)
			&pFilterUnk,            // Array of object pointers. 
			caGUID.cElems,          // Number of property pages
			caGUID.pElems,          // Array of property page CLSIDs
			0,                      // Locale identifier
			0, NULL                 // Reserved
		);

		// Clean up.
		pFilterUnk->Release();
		FilterInfo.pGraph->Release();
		CoTaskMemFree(caGUID.pElems);
	}

	if(FAILED(hr)){
		printf("QueryInterface failed\n");
	}

	MSG msg = { 0 };
	while(GetMessage(&msg, 0, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	CoUninitialize();
	return 0;
}