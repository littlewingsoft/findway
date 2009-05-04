//--------------------------------------------------------------------------------------
// File: findway.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <atlbase.h>
#include <atlhost.h>
#include <iostream>
#include <sstream>
#include <map>

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTCamera.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"
#include "resource.h"

#include "findWay.h"

struct PickInfo
{
	int cellIndex;
	D3DXVECTOR3 pos;
};

CFirstPersonCamera      g_Camera;               // Camera
std::wstring g_stPickingTriIndex=L"None Picked";
std::wstring g_stMousePos = L"Mouse";
std::wstring g_stPickPos = L"Pick";
std::wstring g_strCalcTime = L"n mSec";
std::wstring g_stNeighBorIndex[3]={ L"-1" , L"-1", L"-1"};



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
// agent
//--------------------------------------------------------------------------------------
#define agent_state_stop 0x01
#define agent_state_move 0x02
#define agent_speed 0.15f
int		g_AgentState = agent_state_stop;
int		g_nCurrentPathIndex;

static std::list<PickInfo> pickList;
std::vector<D3DXVECTOR3> g_pathList;
int	g_nCurrentCellIndex = -1;

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

void RenderMesh( LPDIRECT3DDEVICE9 lpDev, const fw::fwMesh& mesh, D3DCOLOR argb=D3DCOLOR_ARGB(0xff,0xff,0xff,0xff) , bool bDrawEdge = true);


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
    
	float fmoverate = (fw::fFar - fw::fNear) * 0.202f ;
	g_Camera.SetScalers( 0.01f, fmoverate );
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
	
	g_pTxtHelper->DrawTextLine( g_stPickingTriIndex.c_str() );
	g_pTxtHelper->DrawTextLine( g_stMousePos.c_str() );
	g_pTxtHelper->DrawTextLine( g_stPickPos.c_str() );
	g_pTxtHelper->DrawTextLine( g_strCalcTime .c_str() );
	
//	g_pTxtHelper->DrawTextLine( g_stNeighBorIndex[0].c_str() );
//		g_pTxtHelper->DrawTextLine( g_stNeighBorIndex[1].c_str() );
//			g_pTxtHelper->DrawTextLine( g_stNeighBorIndex[2].c_str() );
	

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

#define WEB_SCREEN_LEFT		100
#define WEB_SCREEN_TOP		100
#define WEB_SCREEN_RIGHT	700
#define WEB_SCREEN_BOTTOM	500


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
    g_Camera.SetProjParams( D3DXToRadian( fw::fcamFov ), fAspectRatio, fw::fNear, fw::fFar  );

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
	g_Camera.SetProjParams( D3DXToRadian( fw::fcamFov ), fAspectRatio, fw::fNear, fw::fFar );
    //g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width-170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width-170, pBackBufferSurfaceDesc->Height-350 );
    g_SampleUI.SetSize( 170, 300 );


    return S_OK;
}

// line 은 bias 가 안먹는다. 뷰포트 변환으로 해결?

void Render_Line( LPDIRECT3DDEVICE9 lpDev, const std::vector< D3DXVECTOR3 >& posList )//fw::Vertex
	{
		if( posList.empty() )
			return;

D3DVIEWPORT9 mViewport;   // 뷰포트 데이터 저장
D3DVIEWPORT9 mNewViewport; // 새로운 뷰포트 데이터 저장

// 뷰포트에 의해 사용되는 전역 변수 
// 다음 식을 사용해서 1 의 ZBIAS 값을 하드코딩 
//          MinZ - 256/(2^24-1) 과
//          MaxZ - 256/(2^24-1)
// 2^24 는 Zbits 의 개수로부터 나온다. 그리고  256 은  
// Intel (R) Integrated Graphics 를 위해 동작한다, 

// 그러나 그것은 16의 배수라면 어떤 것이라도 상관없다.
float g_fViewportBias   = 0.00000152588f;
//float g_fViewportBias = 0.0000152588f;

// 투영 행렬이 적용된다.
// 뷰 행렬이 적용된다.
lpDev->GetViewport(&mViewport);


// 이전의 뷰포트를 새로운 뷰포트에 복사.
mNewViewport = mViewport;

// bias 변경
mNewViewport.MinZ -= g_fViewportBias; 
mNewViewport.MaxZ -= g_fViewportBias; 


		WORD tmpIndex[4096]={0,};
		for( size_t n=0; n<posList.size()*2; n += 2 )
		{
			if( n==0 )
			tmpIndex[0] = 0;
			else
			tmpIndex[n] =(WORD) n/2;

			tmpIndex[n+1] = tmpIndex[n]+1;
		}

		D3DXMATRIXA16 matWorld;
		D3DXMatrixIdentity( &matWorld );
		lpDev->SetTransform( D3DTS_WORLD, &fw::GetNaviMesh().LocalMat); //
		lpDev->SetFVF( D3DFVF_XYZ );//| D3DFVF_DIFFUSE 
		lpDev->SetViewport( &mNewViewport );

	    lpDev->SetRenderState( D3DRS_LIGHTING, TRUE );

		lpDev->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		lpDev->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA ); //
		lpDev->SetRenderState( D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA ); //  

		lpDev->SetRenderState(D3DRS_TEXTUREFACTOR , D3DCOLOR_ARGB(0xff,0xff,0x00, 0x00)); 

		lpDev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);    
		lpDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);

//		lpDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CURRENT);    // 투명도 적용됨.
//		lpDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1); 

		lpDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);    // 투명도 적용됨.
		lpDev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);    // 투명도 적용됨.
		lpDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2); 


		lpDev->DrawIndexedPrimitiveUP( D3DPT_LINELIST, 0, posList.size(),   posList.size()-1, tmpIndex, D3DFMT_INDEX16,
			(const void*)&(*(posList.begin())), sizeof( D3DXVECTOR3 ) );

		lpDev->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		lpDev->SetTextureStageState(0, D3DTSS_COLOROP,  D3DTOP_MODULATE ); 
		lpDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE); //D3DTA_CURRENT
		lpDev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT  ); //

		lpDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1); // D3DTOP_MODULATE
		lpDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR); //D3DTA_CURRENT
		lpDev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_CURRENT); //D3DTA_CURRENT

		lpDev->SetViewport( &mViewport );
	    lpDev->SetRenderState( D3DRS_LIGHTING, FALSE );
	}

		using namespace fw;
	inline DWORD F2DW( FLOAT f ) { return *((DWORD*)&f); }
	void RenderMesh_NoDoubleBlending( LPDIRECT3DDEVICE9 lpDev, const fw::fwMesh& mesh, DWORD argb )
	{

		float fZSlopeScale = -0.5f;
		float fDepthBias   = 0.0f;
		lpDev->SetTransform( D3DTS_WORLD, &mesh.LocalMat );
		lpDev->SetFVF( D3DFVF_XYZ | D3DFVF_DIFFUSE);//

		int TriCnt = mesh.TriBuffer.size();
		int vtxCnt = mesh.VtxBuffer.size();

		// Turn off depth bias
		lpDev->SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, F2DW(0.0f) );
		lpDev->SetRenderState( D3DRS_DEPTHBIAS, F2DW(0.0f) );

		lpDev->SetRenderState( D3DRS_ZENABLE, 1 );
		lpDev->SetRenderState( D3DRS_ZWRITEENABLE, 1 );
		lpDev->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
		lpDev->SetRenderState( D3DRS_LIGHTING, FALSE );
		lpDev->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );

		lpDev->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		lpDev->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA ); //
		lpDev->SetRenderState( D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA ); //  

		lpDev->SetRenderState(D3DRS_TEXTUREFACTOR , argb ); //D3DCOLOR_ARGB( 0x3f,0xff,0xff, 0xff)

		lpDev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);    
		lpDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);

		lpDev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);    // 투명도 적용됨.
		lpDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2); 


		//lpDev->SetRenderState( D3DRS_STENCIL


		lpDev->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0,  vtxCnt, TriCnt, (const void*)&(*(mesh.TriBuffer.begin())), 
			D3DFMT_INDEX16, (const void*)&(*(mesh.VtxBuffer.begin())), sizeof(fw::Vertex));//

		lpDev->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		lpDev->SetTextureStageState(0, D3DTSS_COLOROP,  D3DTOP_MODULATE ); 
		lpDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE); //D3DTA_CURRENT
		lpDev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT  ); //

		lpDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1); // D3DTOP_MODULATE
		lpDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR); //D3DTA_CURRENT
		lpDev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_CURRENT); //D3DTA_CURRENT
		

		// Turn on depth bias
		lpDev->SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, F2DW(fZSlopeScale) );
		lpDev->SetRenderState( D3DRS_DEPTHBIAS, F2DW(fDepthBias) );

		lpDev->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
		lpDev->SetRenderState( D3DRS_LIGHTING, TRUE );
		lpDev->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
		lpDev->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0,  vtxCnt, TriCnt, (const void*)&(*(mesh.TriBuffer.begin())), 
			D3DFMT_INDEX16, (const void*)&(*(mesh.VtxBuffer.begin())), sizeof(fw::Vertex));//

	}






	void RenderMesh( LPDIRECT3DDEVICE9 lpDev, const fw::fwMesh& mesh, DWORD argb , bool bDrawEdge)
	{
		float fZSlopeScale = -0.5f;
		float fDepthBias   = 0.0f;
		//D3DXMATRIXA16 matWorld;
		//D3DXMatrixIdentity( &matWorld );
		lpDev->SetTransform( D3DTS_WORLD, &mesh.LocalMat );

		lpDev->SetFVF( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE);//

		int TriCnt = mesh.TriBuffer.size();
		int vtxCnt = mesh.VtxBuffer.size();

		// Turn off depth bias
		lpDev->SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, F2DW(0.0f) );
		lpDev->SetRenderState( D3DRS_DEPTHBIAS, F2DW(0.0f) );

		lpDev->SetRenderState( D3DRS_ZENABLE, 1 );
		lpDev->SetRenderState( D3DRS_ZWRITEENABLE, 1 );
		lpDev->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
		lpDev->SetRenderState( D3DRS_LIGHTING, TRUE );
		lpDev->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );

		lpDev->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		lpDev->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA ); //
		lpDev->SetRenderState( D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA ); //  

		lpDev->SetRenderState(D3DRS_TEXTUREFACTOR , argb ); //D3DCOLOR_ARGB( 0x3f,0xff,0xff, 0xff)

		lpDev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);    
		lpDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);

		lpDev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);    // 투명도 적용됨.
		lpDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2); 

		lpDev->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0,  vtxCnt, TriCnt, (const void*)&(*(mesh.TriBuffer.begin())), 
			D3DFMT_INDEX16, (const void*)&(*(mesh.VtxBuffer.begin())), sizeof(fw::Vertex));//

		lpDev->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		lpDev->SetTextureStageState(0, D3DTSS_COLOROP,  D3DTOP_MODULATE ); 
		lpDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE); //D3DTA_CURRENT
		lpDev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT  ); //

		lpDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1); // D3DTOP_MODULATE
		lpDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR); //D3DTA_CURRENT
		lpDev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_CURRENT); //D3DTA_CURRENT
		

		if( bDrawEdge )
		{
			// Turn on depth bias
			lpDev->SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, F2DW(fZSlopeScale) );
			lpDev->SetRenderState( D3DRS_DEPTHBIAS, F2DW(fDepthBias) );

			lpDev->SetRenderState( D3DRS_LIGHTING, TRUE );
			lpDev->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
			lpDev->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0,  vtxCnt, TriCnt, (const void*)&(*(mesh.TriBuffer.begin())), 
				D3DFMT_INDEX16, (const void*)&(*(mesh.VtxBuffer.begin())), sizeof(fw::Vertex));//

			lpDev->SetRenderState( D3DRS_LIGHTING, FALSE );
			lpDev->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
		}

	}





//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );
}

void DrawCellInfo();

HRESULT OnRenderPrimitive(IDirect3DDevice9* lpDev, double fTime, float fElapsedTime, void* pUserContext)
{
	RenderMesh( lpDev, fw::GetNaviMesh() );

	try
	{
		if( g_pathList.empty() == false )
		{
			fw::GetMesh("point").LocalMat._41 = g_pathList[0].x;
			fw::GetMesh("point").LocalMat._42 = g_pathList[0].y;
			fw::GetMesh("point").LocalMat._43 = g_pathList[0].z;


			RenderMesh( lpDev, fw::GetMesh("point") );

			fw::GetMesh("point").LocalMat._41 = g_pathList[g_pathList.size()-1].x;
			fw::GetMesh("point").LocalMat._42 = g_pathList[g_pathList.size()-1].y;
			fw::GetMesh("point").LocalMat._43 = g_pathList[g_pathList.size()-1].z;
			RenderMesh( lpDev, fw::GetMesh("point") );

			for( size_t n=1; n< g_pathList.size()-1; n++)
			{
				fw::GetMesh("point").LocalMat._41 = g_pathList[n].x;
				fw::GetMesh("point").LocalMat._42 = g_pathList[n].y;
				fw::GetMesh("point").LocalMat._43 = g_pathList[n].z;

				fw::GetMesh("point").LocalMat._11 = 0.5f;
				fw::GetMesh("point").LocalMat._22 = 0.5f;
				fw::GetMesh("point").LocalMat._33 = 0.5f;
				//D3DXMatrixTranslation( &fw::GetMesh("agent").LocalMat,  g_pathList[n].x, g_pathList[n].y, g_pathList[n].z );

				RenderMesh( lpDev, fw::GetMesh("point"), D3DCOLOR_ARGB(0x8f,0x0,0x0,0x0) );
			}

				fw::GetMesh("point").LocalMat._11 = 1.0f;
				fw::GetMesh("point").LocalMat._22 = 1.0f;
				fw::GetMesh("point").LocalMat._33 = 1.0f;
			Render_Line(lpDev, g_pathList );
		}
	}
	catch( std::exception ex )
	{
		__asm int 3;
	}

	
	if( g_pathList.empty() ==false )
	{
		static float lerp = 0.0f;
		lerp += agent_speed;
		if( lerp >1.0f )
		{
			lerp = 0.0f;

			if( g_pathList.empty() == false )
				g_pathList.erase( g_pathList.begin() );
		}

		if( g_pathList.size() > 1 )
		{
		const D3DXVECTOR3& pos = g_pathList[0] + (( g_pathList[1]-g_pathList[0])* lerp ) ;

		fw::GetMesh( "agent" ).LocalMat._41 = pos.x;
		fw::GetMesh( "agent" ).LocalMat._42 = pos.y;
		fw::GetMesh( "agent" ).LocalMat._43 = pos.z;
		}
	}

		RenderMesh( lpDev, fw::GetMesh( "agent" ), 0xffffffff, false );

	//DrawCellInfo();

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

	struct RAY
	{
		D3DXVECTOR3 vPickRayDir;
		D3DXVECTOR3 vPickRayOrig;
	};


	RAY MakeRay()
	{
		POINT ptCursor;
		GetCursorPos(&ptCursor);         // 마우스 좌표를 얻어온다!
		ScreenToClient(DXUTGetHWND(), &ptCursor);      // 클라이언트의 윈도우 내의 좌표로 변환! 

		float px = 0.0f;
		float py = 0.0f;

		//2D좌표
   		std::basic_ostringstream<wchar_t> otstrm;
		otstrm << "[Mouse]" << ptCursor.x << ", " << ptCursor.y ;
		g_stMousePos = otstrm.str();

		D3DVIEWPORT9 vp;
		DXUTGetD3D9Device()->GetViewport(&vp);

		D3DXMATRIXA16 proj;
		DXUTGetD3D9Device()->GetTransform(D3DTS_PROJECTION,&proj);

		px = ((( 2.0f * ptCursor.x) / vp.Width) -1.0f ) / proj(0,0);
		py = (((-2.0f * ptCursor.y) / vp.Height) + 1.0f) / proj(1,1);

		RAY ray;
		ray.vPickRayOrig    = D3DXVECTOR3(0.0f,0.0f,0.0f);
		ray.vPickRayDir = D3DXVECTOR3(px,py,1.0f);

		return ray;
	}

	void TransformRay(RAY* ray,D3DXMATRIXA16* T)
	{
		// transform the ray's origin, w = 1
		D3DXVec3TransformCoord(&ray->vPickRayOrig,&ray->vPickRayOrig,T);
		// transform the ray's direction, w = 0;
		D3DXVec3TransformNormal(&ray->vPickRayDir,&ray->vPickRayDir,T);
		D3DXVec3Normalize(&ray->vPickRayDir,&ray->vPickRayDir);
	}


	 // 요곤 DX에 있는 폴리곤 하나와 레이의 충돌 검사 함수!

bool IntersectTriangle( const RAY& ray, D3DXVECTOR3& v0, D3DXVECTOR3& v1, D3DXVECTOR3& v2, float* fDistance, float* u, float* v )
{
	// Find vectors for two edges sharing vert0
	D3DXVECTOR3 edge1 = v1 - v0;
	D3DXVECTOR3 edge2 = v2 - v0;

	// Begin calculating determinant - also used to calculate U parameter
	D3DXVECTOR3 pvec;
	D3DXVec3Cross( &pvec, &ray.vPickRayDir, &edge2 );

	// If determinant is near zero, ray lies in plane of triangle
	FLOAT det = D3DXVec3Dot( &edge1, &pvec );

	D3DXVECTOR3 tvec;
	if( det > 0 )
	{
		tvec = ray.vPickRayOrig - v0;
	}
	else
	{
		tvec = v0 - ray.vPickRayOrig;
		det = -det;
	}

	if( det < 0.0001f )
		return false;

	// Calculate U parameter and test bounds
	*u = D3DXVec3Dot( &tvec, &pvec );
	if( *u < 0.0f || *u > det )
		return false;

	// Prepare to test V parameter
	D3DXVECTOR3 qvec;
	D3DXVec3Cross( &qvec, &tvec, &edge1 );

	// Calculate V parameter and test bounds
	*v = D3DXVec3Dot( &ray.vPickRayDir, &qvec );
	if( *v < 0.0f || *u + *v > det )
		return false;

	// Calculate t, scale parameters, ray intersects triangle
	*fDistance = D3DXVec3Dot( &edge2, &qvec );
	FLOAT fInvDet = 1.0f / det;
	*fDistance *= fInvDet;
	*u *= fInvDet;
	*v *= fInvDet;

	return true;
}


/*
	cell 의 각 변 edge 의 중점 그려주기.
*/
void DrawCellInfo()
{
//	return ;

	if( g_nCurrentCellIndex == -1 )
		return;

	const fw::fwMesh& kMesh = fw::GetNaviMesh();
	const fw::fwNaviCell& cell = kMesh.CellBuffer.at( g_nCurrentCellIndex );

	for( int n=0; n< 3; n++)
	{
	
	fw::GetMesh("sphere01").LocalMat._41 = cell.neighbor[n].edgeCenter.x;
	fw::GetMesh("sphere01").LocalMat._42 = cell.neighbor[n].edgeCenter.y;
	fw::GetMesh("sphere01").LocalMat._43 = cell.neighbor[n].edgeCenter.z;
	RenderMesh( DXUTGetD3D9Device(), fw::GetMesh("sphere01") );
	}
}

void OnLButtonDown()
{
	g_nCurrentCellIndex = -1;
	RAY ray = MakeRay();

   D3DXMATRIXA16 view;
   DXUTGetD3D9Device()->GetTransform(D3DTS_VIEW, &view);
   
   // View를 World로 변환
   D3DXMATRIXA16 viewInverse;
   D3DXMatrixInverse(&viewInverse,0,&view);
   
   TransformRay(&ray,&viewInverse);

	std::map<float, int> closeIndex;


	g_stPickingTriIndex = L"None Picked ";
	const fwMesh& naviMesh = fw::GetNaviMesh();

	D3DXMATRIX pOut = naviMesh.LocalMat;
	//D3DXMatrixInverse( &pOut, 0, &naviMesh.LocalMat  );
	for( size_t n=0; n< naviMesh.TriBuffer.size(); n++)
	{
		fw::Triangle triIndex = naviMesh.TriBuffer[n];

		D3DXVECTOR3 V0 = naviMesh.VtxBuffer[ triIndex.index0 ].pos;
		D3DXVECTOR3 V1 = naviMesh.VtxBuffer[ triIndex.index1 ].pos;
		D3DXVECTOR3 V2 = naviMesh.VtxBuffer[ triIndex.index2 ].pos;

		D3DXVECTOR3 wV0,wV1,wV2;
		// 로컬좌표에서 이루어진 메시정보를 월드좌표로 변환
		D3DXVec3TransformCoord(&wV0, &V0, &pOut);
		D3DXVec3TransformCoord(&wV1, &V1, &pOut);
		D3DXVec3TransformCoord(&wV2, &V2, &pOut);

		//picking polygon IndexList 를 가지고 있다가 최고 적은 크기 값을 유지시킨다.
		float fBary2,fBary1,fDist;
		if( IntersectTriangle( ray, V0, V1, V2, &fDist, &fBary1, &fBary2 ) )
		{
			closeIndex[ fDist ] = n ;
		}
	}

	if( closeIndex.empty() == false )
	{
		std::map<float, int> ::iterator it = closeIndex.begin();			
		int Index =  (*it).second;
		std::basic_ostringstream<wchar_t> ot;
		ot << Index;

		g_nCurrentCellIndex = Index;

		g_stPickingTriIndex = L"Picked TriIndex [ ";
		g_stPickingTriIndex += ot.str();
		g_stPickingTriIndex += L" ] ";

		D3DXVECTOR3 pickPos = ray.vPickRayOrig + ( ray.vPickRayDir* (*it).first );

		//picking 된곳의 3D좌표
		std::basic_ostringstream<wchar_t> otstrm;
		otstrm << "[Pick]" << pickPos.x << ", " <<
			pickPos.y << ", "<<
			pickPos.z ;
		g_stPickPos = otstrm.str();
	

		PickInfo tmpInfo;
		tmpInfo.cellIndex = Index;
		tmpInfo.pos = pickPos;
		pickList.push_back( tmpInfo );

		fw::GetMesh("point").LocalMat._41 =pickPos.x;
		fw::GetMesh("point").LocalMat._42 =pickPos.y;
		fw::GetMesh("point").LocalMat._43 =pickPos.z;
		//D3DXMatrixTranslation( &, pickPos.x, pickPos.y, pickPos.z );
	}

	if( pickList.size() >= 2 )
	{
		std::list<PickInfo>::iterator it = pickList.end();
		it--;
		const PickInfo& tmpEndInfo = (*it);
		int endCellIndex = tmpEndInfo.cellIndex;
		D3DXVECTOR3 end_pos = tmpEndInfo.pos;

		it = pickList.begin();
		const PickInfo& tmpStartInfo = (*it);
		int startCellIndex = tmpStartInfo.cellIndex;
		D3DXVECTOR3 start_pos = tmpStartInfo.pos;


		DWORD prof_begin = timeGetTime();
		fw::FindWay( startCellIndex, start_pos, endCellIndex, end_pos, g_pathList );
		DWORD prof_end = timeGetTime() - prof_begin;
		std::basic_ostringstream<wchar_t> ostrm;
		ostrm << "[profile] : " << prof_end << "mSec" ;
		g_strCalcTime = ostrm.str();

		pickList.clear();
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
	case WM_PAINT:

		 ValidateRect( hWnd, NULL );
		 return 0;

	case WM_LBUTTONDOWN:
		{
			// Capture the mouse, so if the mouse button is 
			// released outside the window, we'll get the WM_LBUTTONUP message
			DXUTGetGlobalTimer()->Stop();
			SetCapture(hWnd);
			
			OnLButtonDown();

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



