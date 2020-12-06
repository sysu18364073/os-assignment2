#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

const int BUFFER_SIZE = 256;
const int PHYSICAL_MEM_SIZE = 128*256;
const int PAGE_TABLE_SIZE = 256;
const int TLB_SIZE = 16;

struct TLB {
	int pagenum;
	int frame;
};

void TLB_init(struct TLB* tlb) {
	for (int i = 0; i < TLB_SIZE; i++) {
		tlb[i].pagenum = -1;
		tlb[i].frame = -1;
	}
}

int inTLB(int pagenum, struct TLB* tlb) {
	for (int i = 0; i < TLB_SIZE; i++) {
		if (tlb[i].pagenum == pagenum)
			return 1;

	}
	return 0;
}

int inPageTable(int pagenum, int* pagetable) {
	for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
		if (pagetable[pagenum] != -1)
			return 1;
	}
	return 0;
}

int isFull(struct TLB* tlb) {
	for (int i = 0; i < TLB_SIZE; i++) {
		if (tlb[i].pagenum == -1)
			return 0;
	}
	return 1;
}

int PageTableisFull(int* pagetable) {
	int counter = 0;
	for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
			if (pagetable[i] == -1)
				counter++;
		}
	if (counter > 128) {
		return 0;
	}
	return 1;
}

int selectTheOldPage(int* pagenumCounter){//Ѱ�����û�з��ʵ�ҳ��
	int minmum=pagenumCounter[0];
	for(int i=1;i<PAGE_TABLE_SIZE;i++){
		if(pagenumCounter[i]<=minmum)
			minmum=pagenumCounter[i];
	}
	for(int i=1;i<PAGE_TABLE_SIZE;i++){
		if(pagenumCounter[i]==minmum)//��ֵ��С�������û�з��ʵ�
			return i;
	}
}

/*void insert_into_TLB_FIFO(struct TLB* mytlb, int page_num, int frame, int* tlbindex) {
	mytlb[(*tlbindex)].pagenum = page_num;
	mytlb[(*tlbindex)].frame = frame;
	(*tlbindex)++;                    //TLB�û���FIFO���� 
	(*tlbindex) = (*tlbindex) % TLB_SIZE;

}*/

void insert_into_TLB_LRU(struct TLB* mytlb,int pagenum,int frame,int* pagenumCounter){
	if(!isFull(mytlb)){                           //�ж�TLB��û����
				for(int i=0;i<TLB_SIZE;i++){      //���û����ֱ�����ȥ
					if(mytlb[i].pagenum==-1){
						mytlb[i].pagenum=pagenum;
						mytlb[i].frame=frame;
						break;
					}
				}
			}
			else{
				int oldpage;                     //������ˣ�Ѱ�����û�з��ʵ�ҳ������û�
				oldpage=selectTheOldPage(pagenumCounter);
				for(int i=0;i<TLB_SIZE;i++){
					if(mytlb[i].pagenum==oldpage){
						mytlb[i].pagenum=pagenum;
						mytlb[i].frame=frame;
						break;
					}
				}
			}

}

int readFromDisk(int* physical_mem,int pagenum,int* Frame) {
	FILE *disk = fopen("BACKING_STORE.bin", "rb");
	char Address[BUFFER_SIZE*10];
	memset(Address, 0, sizeof(Address));

	fseek(disk, pagenum * 256, SEEK_SET);
	fread(Address, 1, 256, disk);

	for (int i = 0; i < 256; i++) {
		physical_mem[*(Frame) * 256 +i] = Address[i];//�Ѷ��������ݴ��������ڴ�
	}
    fclose(disk);
	*(Frame)+=1;
	
    return *(Frame)-1;
}


int main() {

	FILE *address_file = fopen("addresses.txt", "r");//���ļ�
	char buffer[12];
	int logical_address = 0;

	struct TLB myTLB[TLB_SIZE];//���弰��ʼ��TLB
	TLB_init(myTLB);

	int PageTable[PAGE_TABLE_SIZE];//���弰��ʼ��ҳ��
	memset(PageTable, -1, sizeof(PageTable));

	int Physical_Mem[PHYSICAL_MEM_SIZE];//���弰��ʼ�������ڴ�
	memset(Physical_Mem, 0, sizeof(Physical_Mem));

	int frame;
	int openFrame=0;
	int value = 0;
	int tlbindex = 0;//��FIFO�û�������TLB���±�

	int pagenum_counter[PAGE_TABLE_SIZE];//��LRU�û����������ڼ�¼ҳ���·��ʴ��������飬˭����ֵ��С��˵�����û�з���
	memset(pagenum_counter, 0, sizeof(pagenum_counter));

	int TLBhit = 0;//TLB���д���
	int pageFault = 0;//����ȱҳ����Ĵ���
	int TotalAddr = 0;//�ܵķ��ʴ���
	double tlbHitRate = 0;//TLB������
	double pageFaultRate = 0;//ȱҳ��������

	while (fgets(buffer, 12, address_file) != NULL)//���ļ�����ȡ�ļ����͵������� 
	{
		logical_address = atoi(buffer);
		unsigned char mask = 0xFF;

		unsigned char offset;
		int pageNum;
		pageNum = (logical_address >> 8)&mask;//���߼���ַ��ȡҳ��
		printf("Virtual Address:  %d\t", logical_address);
		offset = logical_address & mask;//���߼���ַ��ȡ֡�� 
		
		pagenum_counter[pageNum]++;//��ҳ�������������һ��
		TotalAddr++;

		if (inTLB(pageNum, myTLB)) {//�ж��Ƿ���TLB��
			TLBhit++;
			for (int i = 0; i < TLB_SIZE; i++) {
				if (myTLB[i].pagenum == pageNum) {
					frame = myTLB[i].frame;//����ڣ���ȡ֡��
				}
			}
		}

		else {
			if (inPageTable(pageNum, PageTable)) {//�������TLB�У��ж��Ƿ���ҳ����
				frame = PageTable[pageNum];//����ڣ���ȡ֡��
			}

			else {
				pageFault++;//�������ҳ���У�ȱҳ����+1
				if (!PageTableisFull(PageTable)) {//���ҳ��û��
                    frame = readFromDisk(Physical_Mem, pageNum, &openFrame);//�Ӷ������ļ��ж�ȡ���ݣ����������ڴ��У������ش����������ڴ����һ֡
                    PageTable[pageNum] = frame;//����ҳ������Page Replacement 
				}
				else {//���ҳ������
					int oldPage = selectTheOldPage(pagenum_counter);
					frame = readFromDisk(Physical_Mem, pageNum, &PageTable[oldPage]);
					PageTable[oldPage] = frame;//����ҳ��������LRU�û�
				}
				
			}
			//insert_into_TLB_FIFO(myTLB, pageNum, frame, &tlbindex);
			insert_into_TLB_LRU(myTLB,pageNum,frame,pagenum_counter);//����TLB

		}
        int ind = ((unsigned char)frame*256) + offset;
		value = *(Physical_Mem + ind);
		printf("Physical Address: %d\t Value=%d\n", ind, value);//��������ַ������Ӧ��ֵ

	}

	fclose(address_file);
	printf("\ntlbhits: %d, pagefaults: %d\n", TLBhit, pageFault);//���TLB���д����ͷ���ȱҳ����
	pageFaultRate = (double)pageFault / (double)TotalAddr;
	tlbHitRate = (double)TLBhit / (double)TotalAddr;

	printf("pfRate: %.3lf, tlbhitRate: %.3lf\n", pageFaultRate, tlbHitRate);//���TLB�����ʺͷ���ȱҳ������
	
	system("pause");
	return 0;
}
