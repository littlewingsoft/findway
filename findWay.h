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


	struct fwNeighborCell
	{
		int NeighborIndex; 
		D3DXVECTOR3 edgeCenter; //ab - bc- ca ��
		float		arrivalCost;//�ﰢ���߽ɿ��� ��ab, bc,ca ������ �߽ɱ��� �Ÿ���. 	
	};

	// ��ã�⿡ ���� ���. �̸� �ͽ����Ϳ��� preCalculation �س��´�. 
	// �ڽ��� ������ �̿����� ������ ��� ��Ƴ���.
	struct fwNaviCell
	{
		D3DXVECTOR3 center;
		fwNeighborCell neighbor[3]; //ab,bc,ca ���� �����ϴ� ��.
	};

	//�������� ��ǥ�������Ͽ� ���� ����ġ�� �����ϰ� 
	//���ε����� �����س��´�.
	struct fwPathNode
	{
		// ���� ���� ��� �θ�� ���� �Ÿ������ ���� �����س��� 
		// ���������� ��θ� ���鶧 �������κ��� �������� �Ž��� �ö󰣴�. �׷��Ƿ� ÷���� �������� �������� �ٲ�� ����.
		int kParentCell_Index;
		// ���� ����Ű�� ��
		int kCurrentCell_Index; 

		// fwNaviCell.edgeCenter �� �ε�����. �ڽ��� �̿��� �����ϴ� edge �� �ε���. 
		// �ڽ��� �θ� �˰� �ִ�. �ڽ��� �θ� �����ɶ� ���õɰ�. ��ã�� �Ҷ����� ��� �ٲ����.
		int kNeighborEdgeIndex; 

		//a* ���� G �� �Ҹ����. ���״�� ���������� ���� �� ���� �����̵���
		float	costFromStart; 

		//a* ���� H�� ���ȴ�. ���״�� ������������ ��. ������ ���� �������� ���� �������� ����ġ �����.
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
			// ��尡 �̹� ������ ������� �ʾ�.
			// �ٸ� �ڽ�Ʈ�� �� ������ �װ��� �����Ŵ.
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
				// ���� �ϴ��� �ؾ���.
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

	void FindWay( const int endTriIndex, const D3DXVECTOR3& end_pos, const int startTriIndex, const D3DXVECTOR3& start_pos, std::vector< D3DXVECTOR3> & g_pathList );

};
