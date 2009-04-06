#pragma once

#include <list>
#include <vector>

/*
	�ܼ��� ������ ���׶����� ǥ���ϱ� ���Ͽ�
	NiRefObject �� NiObject, NiAVObject, NiNode, NiMesh ���� �䳻��.
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
		viewer ������ ���ϰ�
		���߿� ������ �����ǰų� Viewer������ ��������.
	*/

	extern D3DXVECTOR3 camEyePos,camLookAt;
	extern float fcamFov;
	extern float fNear,fFar;

	struct Vertex // d3d ��ǥ�� ����. ȭ�� ��¿�. ����� �뵵 �̿ܿ��� ������ ����.
	{
		D3DXVECTOR3 pos;
		DWORD	Color;
	};

	// ��ã�⿡ ���� ���. �̸� �ͽ����Ϳ��� preCalculation �س��´�. 
	struct fwNaviCell
	{
		D3DXVECTOR3 center;
		int NeighborTri[3]; //ab,bc,ca ���� �����ϴ� ��.
		D3DXVECTOR3 edgeCenter[3]; //ab - bc- ca ��
		float		arrivalCost[3];//�ﰢ���߽ɿ��� ��ab, bc,ca ������ �߽ɱ��� �Ÿ���.
	};

	// Ʈ���̾ޱۿ��� ���ؽ��� �ε����� ������.
	// ��� ���ؽ�,�ε��� ���۴� �� �����ؾߵ�.
	struct Triangle
	{
		WORD index0,index1,index2; 
	};

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
	int NaviMesh_LoadXml(const std::string& name ); 
	inline int NaviMesh_Load(const std::string& name ){ return NaviMesh_LoadXml(name);  }
	void NaviMesh_Release();

	const fwNaviCell& GetNaviCell();
	const fwMesh & GetNaviMesh();
	fwMesh& GetAgentMesh();
	fwMesh& GetPointMesh();
	
	// ������ġ�� ����ġ�� ������ pathList �� ���´�.
	// ���� �ð��̿��� �ɸ��� �񵿱� ó���� �ؾ� �Ϸ���.
	// �ϴ��� �ſ� �ִ��� �����ϰ� simple �ϰ� ������.
	void FindWay( const D3DXVECTOR3& start_pos, const D3DXVECTOR3& end_pos, std::vector< D3DXVECTOR3> & pathList );

};
