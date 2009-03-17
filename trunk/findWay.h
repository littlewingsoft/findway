#pragma once

#include <vector>

namespace fw
{
	/*
		viewer ������ ���ϰ�
		���߿� ������ �����ǰų� Viewer������ ��������.
	*/
	extern D3DXVECTOR3 camEyePos,camLookAt;
	extern float fcamFov;

	class Mesh
	{
	public:
		struct Vertex // d3d ��ǥ�� ����. ȭ�� ��¿�. ����� �뵵 �̿ܿ��� ������ ����.
		{
			D3DXVECTOR3 pos;
			//D3DXVECTOR3 normal;
			DWORD	Color;
		};

		// Ʈ���̾ޱۿ��� ���ؽ��� �ε����� ������.
		// ��� ���ؽ�,�ε��� ���۴� �� �����ؾߵ�.
		struct Triangle
		{
			WORD index0,index1,index2; 
		};

		std::vector< Triangle > TriBuffer;
		std::vector< Vertex > VtxBuffer;
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

	const Mesh & GetNaviMesh();
	
	// ������ġ�� ����ġ�� ������ pathList �� ���´�.
	// ���� �ð��̿��� �ɸ��� �񵿱� ó���� �ؾ� �Ϸ���.
	// �ϴ��� �ſ� �ִ��� �����ϰ� simple �ϰ� ������.
	void FindWay( const D3DXVECTOR3& start_pos, const D3DXVECTOR3& end_pos, std::vector< D3DXVECTOR3> & pathList );

};
