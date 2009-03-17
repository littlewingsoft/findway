//--------------------------------------------------------------------------------------
// File: findway.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTCamera.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"
#include "resource.h"

#include "findWay.h"

CFirstPersonCamera      g_Camera;               // Camera

//#define DEBUG_VS   // Uncomment this line to debug D3D9 vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug D3D9 pixel shaders 

// CAMERA_SIZE is used for clipping camera movement
#define CAMERA_SIZE 0.2f
DWORD g_dwNumIntersections=0;


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg         g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*		g_pTxtHelper = NULL;
CDXUTDialog             g_HUD;                  // dialog for standard controls
CDXUTDialog             g_SampleUI;             // dialog for sample specific controls

// Direct3D 9 resources
ID3DXFont*              g_pFont9 = NULL;        
ID3DXSprite*            g_pSprite9 = NULL;      
ID3DXEffect*            g_pEffect9 = NULL;      

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           2
#define IDC_CHANGEDEVICE        3


//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext );
void    CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void    CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void    CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
bool    CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );

bool    CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void    CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext );
void    CALLBACK OnD3D9LostDevice( void* pUserContext );
void    CALLBACK OnD3D9DestroyDevice( void* pUserContext );

void    InitApp();
void    RenderText();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D10) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    DXUTSetCallbackD3D9DeviceAcceptable( IsD3D9DeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( OnD3D9CreateDevice );
    DXUTSetCallbackD3D9DeviceReset( OnD3D9ResetDevice );
    DXUTSetCallbackD3D9DeviceLost( OnD3D9LostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnD3D9DestroyDevice );
    DXUTSetCallbackD3D9FrameRender( OnD3D9FrameRender );


    // This sample requires a stencil buffer. See the callback function for details.
    CGrowableArray<D3DFORMAT>* pDSFormats = DXUTGetD3D9Enumeration()->GetPossibleDepthStencilFormatList();
    pDSFormats->RemoveAll();
    pDSFormats->Add( D3DFMT_D24S8 );
    pDSFormats->Add( D3DFMT_D24X4S4 );
    pDSFormats->Add( D3DFMT_D15S1 );
    pDSFormats->Add( D3DFMT_D24FS8 );

    InitApp();
    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"findway" );
    DXUTCreateDevice( true, 640, 480 );
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}

//--------------------------------------------------------------------------------------
// Returns true if a particular depth-stencil format can be created and used with
// an adapter format and backbuffer format combination.
BOOL IsDepthFormatOk( D3DFORMAT DepthFormat,
                      D3DFORMAT AdapterFormat,
                      D3DFORMAT BackBufferFormat )
{
    // Verify that the depth format exists
    HRESULT hr = DXUTGetD3D9Object()->CheckDeviceFormat( D3DADAPTER_DEFAULT,
                                                        D3DDEVTYPE_HAL,
                                                        AdapterFormat,
                                                        D3DUSAGE_DEPTHSTENCIL,
                                                        D3DRTYPE_SURFACE,
                                                        DepthFormat );
    if( FAILED( hr ) ) return FALSE;

    // Verify that the backbuffer format is valid
    hr = DXUTGetD3D9Object()->CheckDeviceFormat( D3DADAPTER_DEFAULT,
                                                D3DDEVTYPE_HAL,
                                                AdapterFormat,
                                                D3DUSAGE_RENDERTARGET,
                                                D3DRTYPE_SURFACE,
                                                BackBufferFormat );
    if( FAILED( hr ) ) return FALSE;

    // Verify that the depth format is compatible
    hr = DXUTGetD3D9Object()->CheckDepthStencilMatch( D3DADAPTER_DEFAULT,
                                                     D3DDEVTYPE_HAL,
                                                     AdapterFormat,
                                                     BackBufferFormat,
                                                     DepthFormat );

    return SUCCEEDED(hr);
}

//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 10; 
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22, VK_F2 );

    g_SampleUI.SetCallback( OnGUIEvent ); iY = 10; 

    // Setup the camera
//    D3DXVECTOR3 MinBound( g_MinBound.x + CAMERA_SIZE, g_MinBound.y + CAMERA_SIZE, g_MinBound.z + CAMERA_SIZE );
  //  D3DXVECTOR3 MaxBound( g_MaxBound.x - CAMERA_SIZE, g_MaxBound.y - CAMERA_SIZE, g_MaxBound.z - CAMERA_SIZE );
//    g_Camera.SetClipToBoundary( true, &MinBound, &MaxBound );
    g_Camera.SetEnableYAxisMovement( true );
    g_Camera.SetRotateButtons( false, false, true );
    g_Camera.SetScalers( 0.01f, 9.0f );
	
    g_Camera.SetViewParams( &fw::camEyePos, &fw::camLookAt);
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 5, 5 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );  
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Rejects any D3D9 devices that aren't acceptable to the app by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, 
                                      D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    // Skip backbuffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3D9Object(); 
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                    AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, 
                    D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;

    // No fallback defined by this app, so reject any device that 
    // doesn't support at least ps2.0
    if( pCaps->PixelShaderVersion < D3DPS_VERSION(2,0) )
        return false;


    // Must support stencil buffer
    if( !IsDepthFormatOk( D3DFMT_D24S8,
                          AdapterFormat,
                          BackBufferFormat ) &&
        !IsDepthFormatOk( D3DFMT_D24X4S4,
                          AdapterFormat,
                          BackBufferFormat ) &&
        !IsDepthFormatOk( D3DFMT_D15S1,
                          AdapterFormat,
                          BackBufferFormat ) &&
        !IsDepthFormatOk( D3DFMT_D24FS8,
                          AdapterFormat,
                          BackBufferFormat ) )
        return false;

    return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    if( pDeviceSettings->ver == DXUT_D3D9_DEVICE )
    {
        IDirect3D9 *pD3D = DXUTGetD3D9Object();
        D3DCAPS9 Caps;
        pD3D->GetDeviceCaps( pDeviceSettings->d3d9.AdapterOrdinal, pDeviceSettings->d3d9.DeviceType, &Caps );

        // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
        // then switch to SWVP.
        if( (Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 ||
            Caps.VertexShaderVersion < D3DVS_VERSION(1,1) )
        {
            pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }

        // Debugging vertex shaders requires either REF or software vertex processing 
        // and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
        if( pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF )
        {
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;                            
            pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }
#endif
#ifdef DEBUG_PS
        pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif
    }


  // This sample requires a stencil buffer.
    if( IsDepthFormatOk( D3DFMT_D24S8,
                         pDeviceSettings->d3d9.AdapterFormat,
                         pDeviceSettings->d3d9.pp.BackBufferFormat ) )
        pDeviceSettings->d3d9.pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    else
    if( IsDepthFormatOk( D3DFMT_D24X4S4,
                         pDeviceSettings->d3d9.AdapterFormat,
                         pDeviceSettings->d3d9.pp.BackBufferFormat ) )
        pDeviceSettings->d3d9.pp.AutoDepthStencilFormat = D3DFMT_D24X4S4;
    else
    if( IsDepthFormatOk( D3DFMT_D24FS8,
                         pDeviceSettings->d3d9.AdapterFormat,
                         pDeviceSettings->d3d9.pp.BackBufferFormat ) )
        pDeviceSettings->d3d9.pp.AutoDepthStencilFormat = D3DFMT_D24FS8;
    else
    if( IsDepthFormatOk( D3DFMT_D15S1,
                         pDeviceSettings->d3d9.AdapterFormat,
                         pDeviceSettings->d3d9.pp.BackBufferFormat ) )
        pDeviceSettings->d3d9.pp.AutoDepthStencilFormat = D3DFMT_D15S1;

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( (DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF) ||
            (DXUT_D3D10_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE) )
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that will live through a device reset (D3DPOOL_MANAGED)
// and aren't tied to the back buffer size
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D9CreateDevice( pd3dDevice ) );
    V_RETURN( g_SettingsDlg.OnD3D9CreateDevice( pd3dDevice ) );
    
    V_RETURN( D3DXCreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, 
                              OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                              L"Arial", &g_pFont9 ) );

    // Read the D3DX effect file
    WCHAR str[MAX_PATH];
    DWORD dwShaderFlags = D3DXFX_NOT_CLONEABLE;
    #ifdef DEBUG_VS
        dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
    #endif
    #ifdef DEBUG_PS
        dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
    #endif
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"findway.fx" ) );
    V_RETURN( D3DXCreateEffectFromFile( pd3dDevice, str, NULL, NULL, dwShaderFlags, 
                                        NULL, &g_pEffect9, NULL ) );

    // Setup the camera's view parameters
    g_Camera.SetViewParams( &fw::camEyePos, &fw::camLookAt);
    float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DXToRadian( fw::fcamFov ), fAspectRatio, 0.1f, 1000.0f );

	
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that won't live through a device reset (D3DPOOL_DEFAULT) 
// or that are tied to the back buffer size 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, 
                                    const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D9ResetDevice() );
    V_RETURN( g_SettingsDlg.OnD3D9ResetDevice() );

    if( g_pFont9 ) V_RETURN( g_pFont9->OnResetDevice() );
    if( g_pEffect9 ) V_RETURN( g_pEffect9->OnResetDevice() );

    V_RETURN( D3DXCreateSprite( pd3dDevice, &g_pSprite9 ) );
    g_pTxtHelper = new CDXUTTextHelper( g_pFont9, g_pSprite9, NULL, NULL, 15 );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DXToRadian( fw::fcamFov ), fAspectRatio, 0.1f, 1000.0f );
    //g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width-170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width-170, pBackBufferSurfaceDesc->Height-350 );
    g_SampleUI.SetSize( 170, 300 );

    return S_OK;
}

		using namespace fw;
	inline DWORD F2DW( FLOAT f ) { return *((DWORD*)&f); }
	void RenderMesh( LPDIRECT3DDEVICE9 lpDev )
	{

		float fZSlopeScale = -0.5f;
		float fDepthBias   = 0.0f;
		D3DXMATRIXA16 matWorld;
		D3DXMatrixIdentity( &matWorld );
		lpDev->SetTransform( D3DTS_WORLD, &matWorld );

		lpDev->SetFVF( D3DFVF_XYZ | D3DFVF_DIFFUSE);//
		int TriCnt = fw::GetNaviMesh().TriBuffer.size();
		std::vector< fw::Mesh::Triangle>::const_iterator triIt= fw::GetNaviMesh().TriBuffer.begin();

//		fw::Mesh::Triangle pkTri[4096];
//		std::copy( fw::GetNaviMesh().TriBuffer.begin(), fw::GetNaviMesh().TriBuffer.end(), pkTri );
		int vtxCnt = fw::GetNaviMesh().VtxBuffer.size();

//		fw::Mesh::Vertex pkVtx[4096];
//		std::vector<fw::Mesh::Vertex>::const_iterator it = fw::GetNaviMesh().VtxBuffer.begin();
//		std::copy( fw::GetNaviMesh().VtxBuffer.begin(), fw::GetNaviMesh().VtxBuffer.end(), pkVtx );


		// Turn off depth bias
		lpDev->SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, F2DW(0.0f) );
		lpDev->SetRenderState( D3DRS_DEPTHBIAS, F2DW(0.0f) );

		lpDev->SetRenderState( D3DRS_ZENABLE, 1 );
		lpDev->SetRenderState( D3DRS_ZWRITEENABLE, 1 );
		lpDev->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	    lpDev->SetRenderState( D3DRS_LIGHTING, FALSE );
		lpDev->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
		lpDev->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0,  vtxCnt, TriCnt, (const void*)&(*(GetNaviMesh().TriBuffer.begin())), 
			D3DFMT_INDEX16, (const void*)&(*(GetNaviMesh().VtxBuffer.begin())), sizeof(Mesh::Vertex));//

		// Turn on depth bias
		lpDev->SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, F2DW(fZSlopeScale) );
		lpDev->SetRenderState( D3DRS_DEPTHBIAS, F2DW(fDepthBias) );

		lpDev->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	    lpDev->SetRenderState( D3DRS_LIGHTING, TRUE );
		lpDev->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
		lpDev->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0,  vtxCnt, TriCnt, (const void*)&(*(GetNaviMesh().TriBuffer.begin())), 
			D3DFMT_INDEX16, (const void*)&(*(GetNaviMesh().VtxBuffer.begin())), sizeof(Mesh::Vertex));//

	}

//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );
}


HRESULT OnRenderPrimitive(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext)
{
	RenderMesh( pd3dDevice );

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D9 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    HRESULT hr;
    D3DXMATRIXA16 mWorld;
    D3DXMATRIXA16 mView;
    D3DXMATRIXA16 mProj;
    D3DXMATRIXA16 mWorldViewProjection;
    
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }

    // Clear the render target and the zbuffer 
    V( pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL , D3DCOLOR_ARGB(0, 45, 50, 170), 1.0f, 0) );//| 

    // Render the scene
    if( SUCCEEDED( pd3dDevice->BeginScene() ) )
    {
        // Get the projection & view matrix from the camera class
        mWorld =*g_Camera.GetWorldMatrix();
        mProj = *g_Camera.GetProjMatrix();
        mView = *g_Camera.GetViewMatrix();

        mWorldViewProjection = mWorld * mView * mProj;


        // Update the effect's variables.  Instead of using strings, it would 
        // be more efficient to cache a handle to the parameter by calling 
        // ID3DXEffect::GetParameterByName
        V( g_pEffect9->SetMatrix( "g_mWorldViewProjection", &mWorldViewProjection ) );
        V( g_pEffect9->SetMatrix( "g_mWorld", &mWorld ) );
        V( g_pEffect9->SetFloat( "g_fTime", (float)fTime ) );

		// For our world matrix, we will just rotate the object about the y-axis.
		D3DXMATRIXA16 matWorld;

	// Set up the rotation matrix to generate 1 full rotation (2*PI radians) 
	// every 1000 ms. To avoid the loss of precision inherent in very high 
	// floating point numbers, the system time is modulated by the rotation 
	// period before conversion to a radian angle.
		D3DXMATRIXA16 matModelWorld;
		D3DXMatrixIdentity( &matModelWorld );
		pd3dDevice->SetTransform( D3DTS_WORLD, &matModelWorld );
		pd3dDevice->SetTransform( D3DTS_VIEW, g_Camera.GetViewMatrix() );
		pd3dDevice->SetTransform( D3DTS_PROJECTION, g_Camera.GetProjMatrix() );
		V( OnRenderPrimitive( pd3dDevice, fTime, fElapsedTime, pUserContext) );
		

        DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" ); // These events are to help PIX identify what the code is doing
        RenderText();
        V( g_HUD.OnRender( fElapsedTime ) );
        V( g_SampleUI.OnRender( fElapsedTime ) );
        DXUT_EndPerfEvent();



        V( pd3dDevice->EndScene() );
    }
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );


    switch( uMsg )
	{
	case WM_LBUTTONDOWN:
		{
			// Capture the mouse, so if the mouse button is 
			// released outside the window, we'll get the WM_LBUTTONUP message
			DXUTGetGlobalTimer()->Stop();
			SetCapture(hWnd);
			return TRUE;
		}

	case WM_LBUTTONUP:
		{
			ReleaseCapture();
			DXUTGetGlobalTimer()->Start();
			break;
		}

	case WM_CAPTURECHANGED:
		{
			if( (HWND)lParam != hWnd )
			{
				ReleaseCapture();
				DXUTGetGlobalTimer()->Start();
			}
			break;
		}
	}


    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN: DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:        DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:     g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;
    }
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9ResetDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9LostDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D9LostDevice();
    g_SettingsDlg.OnD3D9LostDevice();
    if( g_pFont9 ) g_pFont9->OnLostDevice();
    if( g_pEffect9 ) g_pEffect9->OnLostDevice();
    SAFE_RELEASE( g_pSprite9 );
    SAFE_DELETE( g_pTxtHelper );
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9CreateDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D9DestroyDevice();
    g_SettingsDlg.OnD3D9DestroyDevice();
    SAFE_RELEASE( g_pEffect9 );
    SAFE_RELEASE( g_pFont9 );
}



/*
//--------------------------------------------------------------------------------------
// Checks if mouse point hits geometry the scene.
//--------------------------------------------------------------------------------------
HRESULT Pick()
{
    HRESULT hr;
    D3DXVECTOR3 vPickRayDir;
    D3DXVECTOR3 vPickRayOrig;
    IDirect3DDevice9* pD3Device = DXUTGetD3D9Device();
    const D3DSURFACE_DESC* pd3dsdBackBuffer = DXUTGetD3D9BackBufferSurfaceDesc();

    g_dwNumIntersections = 0L;

    // Get the Pick ray from the mouse position
    if( GetCapture() )
    {
        const D3DXMATRIX *pmatProj = g_Camera.GetProjMatrix();

        POINT ptCursor;
        GetCursorPos( &ptCursor );
        ScreenToClient( DXUTGetHWND(), &ptCursor );

        // Compute the vector of the Pick ray in screen space
        D3DXVECTOR3 v;
        v.x =  ( ( ( 2.0f * ptCursor.x ) / pd3dsdBackBuffer->Width  ) - 1 ) / pmatProj->_11;
        v.y = -( ( ( 2.0f * ptCursor.y ) / pd3dsdBackBuffer->Height ) - 1 ) / pmatProj->_22;
        v.z =  1.0f;

        // Get the inverse view matrix
        const D3DXMATRIX matView = *g_Camera.GetViewMatrix();
        const D3DXMATRIX matWorld = *g_Camera.GetWorldMatrix();
        D3DXMATRIX mWorldView = matWorld * matView;
        D3DXMATRIX m;
        D3DXMatrixInverse( &m, NULL, &mWorldView );

        // Transform the screen space Pick ray into 3D space
        vPickRayDir.x  = v.x*m._11 + v.y*m._21 + v.z*m._31;
        vPickRayDir.y  = v.x*m._12 + v.y*m._22 + v.z*m._32;
        vPickRayDir.z  = v.x*m._13 + v.y*m._23 + v.z*m._33;
        vPickRayOrig.x = m._41;
        vPickRayOrig.y = m._42;
        vPickRayOrig.z = m._43;
    }

    // Get the Picked triangle
    if( GetCapture() )
    {
        LPD3DXMESH              pMesh;
        
		fw::GetMesh() 

        if( g_bUseD3DXIntersect )
        {
            // When calling D3DXIntersect, one can get just the closest intersection and not
            // need to work with a D3DXBUFFER.  Or, to get all intersections between the ray and 
            // the mesh, one can use a D3DXBUFFER to receive all intersections.  We show both
            // methods.
            if( !g_bAllHits )
            {
                // Collect only the closest intersection
                BOOL bHit;
                DWORD dwFace;
                FLOAT fBary1, fBary2, fDist;
                D3DXIntersect(pMesh, &vPickRayOrig, &vPickRayDir, &bHit, &dwFace, &fBary1, &fBary2, &fDist, 
                    NULL, NULL);
                if( bHit )
                {
                    g_dwNumIntersections = 1;
                    g_IntersectionArray[0].dwFace = dwFace;
                    g_IntersectionArray[0].fBary1 = fBary1;
                    g_IntersectionArray[0].fBary2 = fBary2;
                    g_IntersectionArray[0].fDist = fDist;
                }
                else
                {
                    g_dwNumIntersections = 0;
                }
            }
            else 
            {
                // Collect all intersections
                BOOL bHit;
                LPD3DXBUFFER pBuffer = NULL;
                D3DXINTERSECTINFO* pIntersectInfoArray;
                if( FAILED( hr = D3DXIntersect(pMesh, &vPickRayOrig, &vPickRayDir, &bHit, NULL, NULL, NULL, NULL, 
                    &pBuffer, &g_dwNumIntersections) ) )
                {
                    SAFE_RELEASE(pMesh);
                    SAFE_RELEASE(pVB);
                    SAFE_RELEASE(pIB);

                    return hr;
                }
                if( g_dwNumIntersections > 0 )
                {
                    pIntersectInfoArray = (D3DXINTERSECTINFO*)pBuffer->GetBufferPointer();
                    if( g_dwNumIntersections > MAX_INTERSECTIONS )
                        g_dwNumIntersections = MAX_INTERSECTIONS;
                    for( DWORD iIntersection = 0; iIntersection < g_dwNumIntersections; iIntersection++ )
                    {
                        g_IntersectionArray[iIntersection].dwFace = pIntersectInfoArray[iIntersection].FaceIndex;
                        g_IntersectionArray[iIntersection].fBary1 = pIntersectInfoArray[iIntersection].U;
                        g_IntersectionArray[iIntersection].fBary2 = pIntersectInfoArray[iIntersection].V;
                        g_IntersectionArray[iIntersection].fDist = pIntersectInfoArray[iIntersection].Dist;
                    }
                }
                SAFE_RELEASE( pBuffer );
            }

        }
        else
        {
            // Not using D3DX
            DWORD dwNumFaces = g_Mesh.GetMesh()->GetNumFaces();
            FLOAT fBary1, fBary2;
            FLOAT fDist;
            for( DWORD i=0; i<dwNumFaces; i++ )
            {
                D3DXVECTOR3 v0 = pVertices[pIndices[3*i+0]].p;
                D3DXVECTOR3 v1 = pVertices[pIndices[3*i+1]].p;
                D3DXVECTOR3 v2 = pVertices[pIndices[3*i+2]].p;

                // Check if the Pick ray passes through this point
                if( IntersectTriangle( vPickRayOrig, vPickRayDir, v0, v1, v2,
                                       &fDist, &fBary1, &fBary2 ) )
                {
                    if( g_bAllHits || g_dwNumIntersections == 0 || fDist < g_IntersectionArray[0].fDist )
                    {
                        if( !g_bAllHits )
                            g_dwNumIntersections = 0;
                        g_IntersectionArray[g_dwNumIntersections].dwFace = i;
                        g_IntersectionArray[g_dwNumIntersections].fBary1 = fBary1;
                        g_IntersectionArray[g_dwNumIntersections].fBary2 = fBary2;
                        g_IntersectionArray[g_dwNumIntersections].fDist = fDist;
                        g_dwNumIntersections++;
                        if( g_dwNumIntersections == MAX_INTERSECTIONS )
                            break;
                    }
                }
            }
        }

        // Now, for each intersection, add a triangle to g_pVB and compute texture coordinates
        if( g_dwNumIntersections > 0 )
        {
            D3DVERTEX* v;
            D3DVERTEX* vThisTri;
            WORD* iThisTri;
            D3DVERTEX  v1, v2, v3;
            INTERSECTION* pIntersection;

            g_pVB->Lock( 0, 0, (void**)&v, 0 );

            for( DWORD iIntersection = 0; iIntersection < g_dwNumIntersections; iIntersection++ )
            {
                pIntersection = &g_IntersectionArray[iIntersection];

                vThisTri = &v[iIntersection * 3];
                iThisTri = &pIndices[3*pIntersection->dwFace];
                // get vertices hit
                vThisTri[0] = pVertices[iThisTri[0]];
                vThisTri[1] = pVertices[iThisTri[1]];
                vThisTri[2] = pVertices[iThisTri[2]];

                // If all you want is the vertices hit, then you are done.  In this sample, we
                // want to show how to infer texture coordinates as well, using the BaryCentric
                // coordinates supplied by D3DXIntersect
                FLOAT dtu1 = vThisTri[1].tu - vThisTri[0].tu;
                FLOAT dtu2 = vThisTri[2].tu - vThisTri[0].tu;
                FLOAT dtv1 = vThisTri[1].tv - vThisTri[0].tv;
                FLOAT dtv2 = vThisTri[2].tv - vThisTri[0].tv;
                pIntersection->tu = vThisTri[0].tu + pIntersection->fBary1 * dtu1 + pIntersection->fBary2 * dtu2;
                pIntersection->tv = vThisTri[0].tv + pIntersection->fBary1 * dtv1 + pIntersection->fBary2 * dtv2;
            }

            g_pVB->Unlock();
        }

    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Given a ray origin (orig) and direction (dir), and three vertices of a triangle, this
// function returns TRUE and the interpolated texture coordinates if the ray intersects 
// the triangle
//--------------------------------------------------------------------------------------
bool IntersectTriangle( const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, 
                        D3DXVECTOR3& v0, D3DXVECTOR3& v1, D3DXVECTOR3& v2, 
                        FLOAT* t, FLOAT* u, FLOAT* v )
{
    // Find vectors for two edges sharing vert0
    D3DXVECTOR3 edge1 = v1 - v0;
    D3DXVECTOR3 edge2 = v2 - v0;

    // Begin calculating determinant - also used to calculate U parameter
    D3DXVECTOR3 pvec;
    D3DXVec3Cross( &pvec, &dir, &edge2 );

    // If determinant is near zero, ray lies in plane of triangle
    FLOAT det = D3DXVec3Dot( &edge1, &pvec );

    D3DXVECTOR3 tvec;
    if( det > 0 )
    {
        tvec = orig - v0;
    }
    else
    {
        tvec = v0 - orig;
        det = -det;
    }

    if( det < 0.0001f )
        return FALSE;

    // Calculate U parameter and test bounds
    *u = D3DXVec3Dot( &tvec, &pvec );
    if( *u < 0.0f || *u > det )
        return FALSE;

    // Prepare to test V parameter
    D3DXVECTOR3 qvec;
    D3DXVec3Cross( &qvec, &tvec, &edge1 );

    // Calculate V parameter and test bounds
    *v = D3DXVec3Dot( &dir, &qvec );
    if( *v < 0.0f || *u + *v > det )
        return FALSE;

    // Calculate t, scale parameters, ray intersects triangle
    *t = D3DXVec3Dot( &edge2, &qvec );
    FLOAT fInvDet = 1.0f / det;
    *t *= fInvDet;
    *u *= fInvDet;
    *v *= fInvDet;

    return TRUE;
}
*/
