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
	�ܼ��� ������ ���׶����� ǥ���ϱ� ���Ͽ�
	NiRefObject �� NiObject, NiAVObject, NiNode, NiMesh ���� �䳻��.
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
//// �ڽĸ���Ʈ�� ������ �˻��Ҽ��ִ�.
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
		viewer ������ ���ϰ�
		���߿� ������ �����ǰų� Viewer������ ��������.
	*/

	extern D3DXVECTOR3 camEyePos,camLookAt;
	extern float fcamFov;
	extern float fNear,fFar;

	struct Vertex // d3d ��ǥ�� ����. ȭ�� ��¿�. ����� �뵵 �̿ܿ��� ������ ����.
	{
		D3DXVECTOR3 pos;
		D3DXVECTOR3 norm;
		DWORD	Color;
	};

	// Ʈ���̾ޱۿ��� ���ؽ��� �ε����� ������.
	// ��� ���ؽ�,�ε��� ���۴� �� �����ؾߵ�.
	struct Triangle
	{
		WORD index0,index1,index2; 
	};


	struct fwEdge
	{
		bool bUsePath;
		int NeighborIndex; 
		D3DXVECTOR3 center; //ab - bc- ca ��
		float		arrivalCost;//�ﰢ���߽ɿ��� ��ab, bc,ca ������ �߽ɱ��� �Ÿ���. 	
		
		fwEdge(): bUsePath(false),NeighborIndex(-1), center(0,0,0),arrivalCost(0.0f)
		{
		}
	};

	// ��ã�⿡ ���� ���. �̸� �ͽ����Ϳ��� preCalculation �س��´�. 
	// �ڽ��� ������ �̿����� ������ ��� ��Ƴ���.
	struct fwNaviCell
	{
		D3DXVECTOR3 center;
		D3DXVECTOR3 normal;// ���� �񱳿� ����.
		fwEdge edge[3]; //ab,bc,ca ���� �����ϴ� ��.
	};

	// ���� ��ΰ� �� ���.
	struct fwPathNode
	{
		D3DXVECTOR3 pos; 
		int cell_Index; 
	};


	//�������� ��ǥ�������Ͽ� ���� ����ġ�� �����ϰ� 
	//���ε����� �����س��´�.
	struct fwHeapNode
	{
		// ���� ���� ��� �θ�� ���� �Ÿ������ ���� �����س��� 
		// ���������� ��θ� ���鶧 �������κ��� �������� �Ž��� �ö󰣴�. �׷��Ƿ� ÷���� �������� �������� �ٲ�� ����.
		int kIndex_ParentCell;

		// �ڽ� ���� �ε���
		int kChildCell_Index;

		// ���� ����Ű�� ��
		int kIndex_CurrentCell; 

		// fwNaviCell.center �� �ε�����. �ڽ��� �̿��� �����ϴ� edge �� �ε���. 
		// �ڽ��� �θ� �˰� �ִ�. �ڽ��� �θ� �����ɶ� ���õɰ�. ��ã�� �Ҷ����� ��� �ٲ����.
		int kIndex_NeighborEdge; 

		//a* ���� G �� �Ҹ����. ���״�� ���������� ���� �� ���� �����̵���
		float	costFromStart; 

		//a* ���� H�� ���ȴ�. ���״�� ������������ ��. ������ ���� �������� ���� �������� ����ġ �����.
		float	costToGoal;	   

		fwHeapNode(): kIndex_ParentCell (-1),kIndex_CurrentCell(-1), kIndex_NeighborEdge(-1), costFromStart(0.0f),costToGoal(0.0f){}
		fwHeapNode( int _kIndex_ParentCell, int _kIndex_CurrentCell, int _kIndex_NeighborEdge, float _costFromStart,float _costToGoal )
		{
			kIndex_ParentCell = _kIndex_ParentCell;
			kIndex_CurrentCell= _kIndex_CurrentCell;
			kIndex_NeighborEdge = _kIndex_NeighborEdge ;
			costFromStart = _costFromStart;
			costToGoal = _costToGoal;
		}

		bool operator < (const fwHeapNode& ref) const
		{
			return GetTotalCost() < ref.GetTotalCost() ;
		}

		bool operator > (const fwHeapNode& ref ) const
		{
			return GetTotalCost() > ref.GetTotalCost();
		}

		bool operator == (const fwHeapNode& ref ) const
		{
			return GetTotalCost() == ref.GetTotalCost() ;
		}


		float GetTotalCost()const { return costToGoal+costFromStart; }
	};

#define USECHECKMAP 0

	class fwPathHeap
	{
		typedef std::vector<fwHeapNode> container;
		container	m_OpenList;
	
#if USECHECKMAP == 1
		typedef std::set<int, std::greater<int> > cont_checkMap; // ���� �ִ��� ���θ� ã�Ƽ� �˻��뵵.
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

		bool Top( fwHeapNode& node ) 
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
				const fwHeapNode& node = *it;
				if( node.kIndex_CurrentCell == cellIndex )
					return true;
				it++;
			}

			return false;
#endif
		}
		void PopHead()
		{
			std::pop_heap( m_OpenList.begin(), m_OpenList.end(), std::greater<fwHeapNode>() );

#if USECHECKMAP == 1
			checkMap.erase(  (*(m_OpenList.end()-1)).kIndex_CurrentCell );
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

		void AddPathNode( const fwHeapNode& node )
		{
			// ��尡 �̹� ������ ������� �ʾ�.
			// �ٸ� �ڽ�Ʈ�� �� ������ �װ��� �����Ŵ.
			if( IsInHeap( node.kIndex_CurrentCell ) )
			{
				container::iterator it = m_OpenList.begin();

				while( it != m_OpenList.end()  )
				{
					fwHeapNode& tmpNode= *it;
					if( tmpNode.kIndex_CurrentCell == node.kIndex_CurrentCell )
					{
						if( tmpNode.GetTotalCost() > node.GetTotalCost() )
							tmpNode= node;
						break;
					}
					it++;
				}
				// ���� �ϴ��� �ؾ���.
				std::make_heap( m_OpenList.begin(), m_OpenList.end() , std::greater<fwHeapNode>() );

#if USECHECKMAP == 1
				checkMap.insert( node.kIndex_CurrentCell );
#endif
			}
			else
			{
				m_OpenList.push_back( node );
				std::push_heap( m_OpenList.begin(), m_OpenList.end(), std::greater<fwHeapNode>() );
#if USECHECKMAP == 1
checkMap.insert( node.kIndex_CurrentCell );
#endif
			}

			//std::sort( m_OpenList.begin(), m_OpenList.end() );			
			
		}
	};

	class fwMesh //ȭ�鿡 ����ϰų� ��ġ���� �������� core �Ѱ�.
	{
	public:

		D3DXMATRIXA16 LocalMat; // �̰� �������ָ� ���尪 ����.
		std::vector< Triangle > TriBuffer;
		std::vector< Vertex > VtxBuffer;
		std::vector< fwNaviCell > CellBuffer;
	};

	/*
		return ��
		0: ����
		1: 
	*/
	int System_Create(); // dll�� ���� �ڷ� �غ�.
	void System_Shutdown(); // �ε��Ǿ����� ��� ����.

	/*
		desc: xml �̳� bin ����Ÿ�� �о���ε� Mesh Format ���� �������Ѵ�.
		return ��
		0: ����.
		1: ������ ã���� ����.
		2: �������̴�( �Ľ̺Ұ� )
	*/
	void AddMesh( const std::string& fileName ); 
	void RemoveMesh( const std::string& meshName );
	void RemoveAllMesh();

	//xml�� bin ���Ϸκ��� �аԲ� �������� ���߿� ���Ϸδ��� ���ϸ� �����̴� �Ҽ��ְ� �߸����� adaptor �ۼ�.
	int AddMesh_FromXml(const std::string& name );  //xml �κ���.
	inline int NaviMesh_Load(const std::string& name ){ return AddMesh_FromXml(name);  } //bin ���Ϸκ���. 

	const fwMesh & GetMesh_const(const std::string & name );
	fwMesh& GetMesh(const std::string & name )  ;
	
	const fwMesh& GetNaviMesh();

	// ������ġ�� ����ġ�� ������ g_pathList �� ���´�.
	// ���� �ð��̿��� �ɸ��� �񵿱� ó���� �ؾ� �Ϸ���.
	// �ϴ��� �ſ� �ִ��� �����ϰ� simple �ϰ� ������.
	void FindWay( const D3DXVECTOR3& start_pos, const D3DXVECTOR3& end_pos, std::vector< D3DXVECTOR3> & g_pathList );

	void FindWay( const int endTriIndex, const D3DXVECTOR3& end_pos, const int startTriIndex, const D3DXVECTOR3& start_pos, std::vector< fwPathNode> & g_pathList );

};
