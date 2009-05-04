#pragma once

#include <algorithm>
#include <list>
#include <set>
#include <vector>


#define PROF_BEGIN() DWORD prof_begin=timeGetTime(); 
#define PROF_END() \
			DWORD prof_end = timeGetTime() - prof_begin;\
		std::basic_ostringstream<wchar_t> ostrm;\
		ostrm << "[profile] : " << prof_end << "mSec\n" ;\
		std::wstring strCalcTime = ostrm.str();\
		OutputDebugString( strCalcTime.c_str() );\

/*
	단순한 구조의 씬그라프를 표현하기 위하여
	NiRefObject 와 NiObject, NiAVObject, NiNode, NiMesh 등을 흉내냄.
*/

namespace fw
{

	//class fwRefObject
	//{
	//	int m_nRefCnt;
	//	fwRefObject():m_nRefCnt(0){}

	//	int GetRefCnt(){ return m_nRefCnt; }

	//	void IncreaseCount()
	//	{
	//		m_nRefCnt++; 
	//	}

	//	void DecreaseCount()
	//	{
	//		m_nRefCnt--; 
	//		if( m_nRefCnt == 0 )
	//			delete this;
	//	}		
	//};
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
		D3DXVECTOR3 norm;
		DWORD	Color;
	};

	// 트라이앵글에는 버텍스의 인덱스만 존재함.
	// 고로 버텍스,인덱스 버퍼는 꼭 존재해야됨.
	struct Triangle
	{
		WORD index0,index1,index2; 
	};


	struct fwNeighborCell
	{
		int NeighborIndex; 
		D3DXVECTOR3 edgeCenter; //ab - bc- ca 순
		float		arrivalCost;//삼각형중심에서 각ab, bc,ca 순으로 중심까지 거리값. 	
	};

	// 길찾기에 쓰일 재료. 미리 익스포터에서 preCalculation 해놓는다. 
	// 자신의 중점과 이웃셀의 정보를 모두 담아놓음.
	struct fwNaviCell
	{
		D3DXVECTOR3 center;
		fwNeighborCell neighbor[3]; //ab,bc,ca 변을 공유하는 순.
	};

	//시작점과 목표점에대하여 최종 가중치를 저장하고 
	//셀인덱스를 저장해놓는다.
	struct fwPathNode
	{
		// 현재 셀이 어느 부모와 가장 거리비용이 싼지 저장해놓고 
		// 최종적으로 경로를 만들때 목적지로부터 시작지로 거슬러 올라간다. 그러므로 첨부터 시작지랑 목적지가 바뀌면 편함.
		int kParentCell_Index;
		// 현재 가르키는 셀
		int kCurrentCell_Index; 

		// fwNaviCell.edgeCenter 의 인덱스값. 자신의 이웃과 공유하는 edge 의 인덱스. 
		// 자식은 부모를 알고 있다. 자신의 부모가 결정될때 셋팅될값. 길찾기 할때마다 계속 바뀔것임.
		int kNeighborEdgeIndex; 

		//a* 에선 G 로 불리운다. 말그대로 시작지부터 현재 셀 까지 누적이동값
		float	costFromStart; 

		//a* 에선 H로 통용된다. 말그대로 목적지까지의 값. 묻지도 말고 따지지도 말고 목적지와 가중치 계산함.
		float	costToGoal;	   

		fwPathNode(): kParentCell_Index (-1),kCurrentCell_Index(-1), kNeighborEdgeIndex(-1), costFromStart(0.0f),costToGoal(0.0f){}
		fwPathNode( int _kParentCell_Index, int _cellIndex, int _kNeighborEdgeIndex, float _costFromStart,float _costToGoal )
		{
			kParentCell_Index = _kParentCell_Index;
			kCurrentCell_Index= _cellIndex ;
			kNeighborEdgeIndex = _kNeighborEdgeIndex ;
			costFromStart = _costFromStart;
			costToGoal = _costToGoal;
		}

		bool operator < (const fwPathNode& ref) const
		{
			return GetTotalCost() < ref.GetTotalCost() ;
		}

		bool operator > (const fwPathNode& ref ) const
		{
			return GetTotalCost() > ref.GetTotalCost();
		}

		bool operator == (const fwPathNode& ref ) const
		{
			return GetTotalCost() == ref.GetTotalCost() ;
		}


		float GetTotalCost()const { return costToGoal+costFromStart; }
	};

#define USECHECKMAP 0

	class fwPathHeap
	{
		typedef std::vector<fwPathNode> container;
		container	m_OpenList;
	
#if USECHECKMAP == 1
		typedef std::set<int, std::greater<int> > cont_checkMap; // 단지 있는지 여부만 찾아서 검색용도.
		cont_checkMap checkMap;
#endif

	public:
		fwPathHeap()
		{
			m_OpenList.clear();
#if USECHECKMAP == 1
			checkMap.clear();
#endif
		}

		bool Top( fwPathNode& node ) 
		{
			if( m_OpenList.empty() )
			{
				return false;
			}

			node= m_OpenList.front();
			return true;
		}
		
		bool IsInHeap( int cellIndex )
		{
			if( m_OpenList.empty() )
				return false;

#if USECHECKMAP == 1
			return checkMap.count( cellIndex ) > 0 ;
#else
			container::iterator it = m_OpenList.begin();
			
			while( it != m_OpenList.end()  )
			{
				const fwPathNode& node = *it;
				if( node.kCurrentCell_Index == cellIndex )
					return true;
				it++;
			}

			return false;
#endif
		}
		void PopHead()
		{
			std::pop_heap( m_OpenList.begin(), m_OpenList.end(), std::greater<fwPathNode>() );

#if USECHECKMAP == 1
			checkMap.erase(  (*(m_OpenList.end()-1)).kCurrentCell_Index );
#endif
			m_OpenList.pop_back();

		}

		void clear()
		{
#if USECHECKMAP == 1
			checkMap.clear();
#endif
			m_OpenList.clear();
		}

		bool empty()
		{
			return m_OpenList.empty();
		}

		void AddPathNode( const fwPathNode& node )
		{
			// 노드가 이미 있으면 집어넣지 않아.
			// 다만 코스트가 더 적으면 그것을 적용시킴.
			if( IsInHeap( node.kCurrentCell_Index ) )
			{
				container::iterator it = m_OpenList.begin();

				while( it != m_OpenList.end()  )
				{
					fwPathNode& tmpNode= *it;
					if( tmpNode.kCurrentCell_Index == node.kCurrentCell_Index )
					{
						if( tmpNode.GetTotalCost() > node.GetTotalCost() )
							tmpNode= node;
						break;
					}
					it++;
				}
				// 정렬 하던가 해야함.
				std::make_heap( m_OpenList.begin(), m_OpenList.end() , std::greater<fwPathNode>() );

#if USECHECKMAP == 1
				checkMap.insert( node.kCurrentCell_Index );
#endif
			}
			else
			{
				m_OpenList.push_back( node );
				std::push_heap( m_OpenList.begin(), m_OpenList.end(), std::greater<fwPathNode>() );
#if USECHECKMAP == 1
checkMap.insert( node.kCurrentCell_Index );
#endif
			}

			//std::sort( m_OpenList.begin(), m_OpenList.end() );			
			
		}
	};

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
	void AddMesh( const std::string& fileName ); 
	void RemoveMesh( const std::string& meshName );
	void RemoveAllMesh();

	//xml과 bin 파일로부터 읽게끔 되있으나 나중엔 파일로더를 통하면 무엇이던 할수있게 중립적인 adaptor 작성.
	int AddMesh_FromXml(const std::string& name );  //xml 로부터.
	inline int NaviMesh_Load(const std::string& name ){ return AddMesh_FromXml(name);  } //bin 파일로부터. 

	const fwMesh & GetMesh_const(const std::string & name );
	fwMesh& GetMesh(const std::string & name )  ;
	
	const fwMesh& GetNaviMesh();

	// 시작위치와 끝위치를 넣으면 g_pathList 가 나온다.
	// 만일 시간이오래 걸리면 비동기 처리를 해야 하려나.
	// 일단은 매우 최대한 간단하고 simple 하게 유지함.
	void FindWay( const D3DXVECTOR3& start_pos, const D3DXVECTOR3& end_pos, std::vector< D3DXVECTOR3> & g_pathList );

	void FindWay( const int endTriIndex, const D3DXVECTOR3& end_pos, const int startTriIndex, const D3DXVECTOR3& start_pos, std::vector< D3DXVECTOR3> & g_pathList );

};
