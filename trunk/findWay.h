#pragma once

#include <list>
#include <vector>

/*
	단순한 구조의 씬그라프를 표현하기 위하여
	NiRefObject 와 NiObject, NiAVObject, NiNode, NiMesh 등을 흉내냄.
*/

namespace fw
{

	class fwRefObject
	{
		int m_nRefCnt;
		fwRefObject():m_nRefCnt(0){}

		int GetRefCnt(){ return m_nRefCnt; }

		void IncreaseCount()
		{
			m_nRefCnt++; 
		}

		void DecreaseCount()
		{
			m_nRefCnt--; 
			if( m_nRefCnt == 0 )
				delete this;
		}		
	};

	/*
		viewer 에서만 쓰일값
		나중에 모조리 삭제되거나 Viewer쪽으로 빠질것임.
	*/

	extern D3DXVECTOR3 camEyePos,camLookAt;
	extern float fcamFov;
	extern float fNear,fFar;

	struct Vertex // d3d 좌표계 기준. 화면 출력용. 디버그 용도 이외에는 쓰이지 않음.
	{
		D3DXVECTOR3 pos;
		DWORD	Color;
	};

	// 길찾기에 쓰일 재료. 미리 익스포터에서 preCalculation 해놓는다. 
	struct fwNaviCell
	{
		D3DXVECTOR3 center;
		int NeighborTri[3]; //ab,bc,ca 변을 공유하는 순.
		D3DXVECTOR3 edgeCenter[3]; //ab - bc- ca 순
		float		arrivalCost[3];//삼각형중심에서 각ab, bc,ca 순으로 중심까지 거리값.
	};

	// 트라이앵글에는 버텍스의 인덱스만 존재함.
	// 고로 버텍스,인덱스 버퍼는 꼭 존재해야됨.
	struct Triangle
	{
		WORD index0,index1,index2; 
	};

	//// 자식리스트를 가지며 검색할수있다.
	//class fwNode: public fwRefObject
	//{
	//	fwNode* pkParent;
	//	std::list<fwNode*> childList;
	//public:
	//	fwNode():pkParent(0){}
	//	~fwNode(){ childList.clear(); }

	//	AttachChildNode( fwNode* pkNode ){ childList.push_back( pkNode ); }
	//	DetachChildNode( fwNode* pkNode ){ childList.erase( std::find( childList.begin(),childList.end(),pkNode ); }

	//	fwNode& GetObjectByName( const std::string& name );
	//	const fwNode& GetObjectByName( const std::string& name );
	//};

	class fwMesh //화면에 출력하거나 위치값을 갖기위한 core 한값.
	{
	public:

		D3DXMATRIXA16 LocalMat; // 이걸 셋팅해주면 월드값 나옴.
		std::vector< Triangle > TriBuffer;
		std::vector< Vertex > VtxBuffer;
		std::vector< fwNaviCell > CellBuffer;
	};

	/*
		return 값
		0: 정상
		1: 
	*/
	int System_Create(); // dll과 각종 자료 준비.
	void System_Shutdown(); // 로딩되었던것 모두 해제.

	/*
		desc: xml 이나 bin 데이타를 읽어들인뒤 Mesh Format 으로 컨버팅한다.
		return 값
		0: 정상.
		1: 파일을 찾을수 없다.
		2: 구버젼이다( 파싱불가 )
	*/
	int NaviMesh_LoadXml(const std::string& name ); 
	inline int NaviMesh_Load(const std::string& name ){ return NaviMesh_LoadXml(name);  }
	void NaviMesh_Release();

	const fwNaviCell& GetNaviCell();
	const fwMesh & GetNaviMesh();
	fwMesh& GetAgentMesh();
	fwMesh& GetPointMesh();
	
	// 시작위치와 끝위치를 넣으면 pathList 가 나온다.
	// 만일 시간이오래 걸리면 비동기 처리를 해야 하려나.
	// 일단은 매우 최대한 간단하고 simple 하게 유지함.
	void FindWay( const D3DXVECTOR3& start_pos, const D3DXVECTOR3& end_pos, std::vector< D3DXVECTOR3> & pathList );

};
