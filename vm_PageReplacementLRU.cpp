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

int selectTheOldPage(int* pagenumCounter){//寻找最近没有访问的页面
	int minmum=pagenumCounter[0];
	for(int i=1;i<PAGE_TABLE_SIZE;i++){
		if(pagenumCounter[i]<=minmum)
			minmum=pagenumCounter[i];
	}
	for(int i=1;i<PAGE_TABLE_SIZE;i++){
		if(pagenumCounter[i]==minmum)//数值最小的是最近没有访问的
			return i;
	}
}

/*void insert_into_TLB_FIFO(struct TLB* mytlb, int page_num, int frame, int* tlbindex) {
	mytlb[(*tlbindex)].pagenum = page_num;
	mytlb[(*tlbindex)].frame = frame;
	(*tlbindex)++;                    //TLB置换，FIFO策略 
	(*tlbindex) = (*tlbindex) % TLB_SIZE;

}*/

void insert_into_TLB_LRU(struct TLB* mytlb,int pagenum,int frame,int* pagenumCounter){
	if(!isFull(mytlb)){                           //判断TLB有没有满
				for(int i=0;i<TLB_SIZE;i++){      //如果没满，直接填进去
					if(mytlb[i].pagenum==-1){
						mytlb[i].pagenum=pagenum;
						mytlb[i].frame=frame;
						break;
					}
				}
			}
			else{
				int oldpage;                     //如果满了，寻找最近没有访问的页面进行置换
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
		physical_mem[*(Frame) * 256 +i] = Address[i];//把读到的数据存入物理内存
	}
    fclose(disk);
	*(Frame)+=1;
	
    return *(Frame)-1;
}


int main() {

	FILE *address_file = fopen("addresses.txt", "r");//打开文件
	char buffer[12];
	int logical_address = 0;

	struct TLB myTLB[TLB_SIZE];//定义及初始化TLB
	TLB_init(myTLB);

	int PageTable[PAGE_TABLE_SIZE];//定义及初始化页表
	memset(PageTable, -1, sizeof(PageTable));

	int Physical_Mem[PHYSICAL_MEM_SIZE];//定义及初始化物理内存
	memset(Physical_Mem, 0, sizeof(Physical_Mem));

	int frame;
	int openFrame=0;
	int value = 0;
	int tlbindex = 0;//在FIFO置换策略中TLB的下标

	int pagenum_counter[PAGE_TABLE_SIZE];//在LRU置换策略中用于记录页最新访问次数的数组，谁的数值最小，说明最近没有访问
	memset(pagenum_counter, 0, sizeof(pagenum_counter));

	int TLBhit = 0;//TLB命中次数
	int pageFault = 0;//发生缺页错误的次数
	int TotalAddr = 0;//总的访问次数
	double tlbHitRate = 0;//TLB命中率
	double pageFaultRate = 0;//缺页错误发生率

	while (fgets(buffer, 12, address_file) != NULL)//从文件流读取文件发送到缓冲区 
	{
		logical_address = atoi(buffer);
		unsigned char mask = 0xFF;

		unsigned char offset;
		int pageNum;
		pageNum = (logical_address >> 8)&mask;//从逻辑地址获取页码
		printf("Virtual Address:  %d\t", logical_address);
		offset = logical_address & mask;//从逻辑地址获取帧码 
		
		pagenum_counter[pageNum]++;//该页面最近被访问了一次
		TotalAddr++;

		if (inTLB(pageNum, myTLB)) {//判断是否在TLB中
			TLBhit++;
			for (int i = 0; i < TLB_SIZE; i++) {
				if (myTLB[i].pagenum == pageNum) {
					frame = myTLB[i].frame;//如果在，获取帧码
				}
			}
		}

		else {
			if (inPageTable(pageNum, PageTable)) {//如果不在TLB中，判断是否在页表中
				frame = PageTable[pageNum];//如果在，获取帧码
			}

			else {
				pageFault++;//如果不在页表中，缺页次数+1
				if (!PageTableisFull(PageTable)) {//如果页表没满
                    frame = readFromDisk(Physical_Mem, pageNum, &openFrame);//从二进制文件中读取数据，存在物理内存中，并返回存在了物理内存的哪一帧
                    PageTable[pageNum] = frame;//更新页表，无需Page Replacement 
				}
				else {//如果页表满了
					int oldPage = selectTheOldPage(pagenum_counter);
					frame = readFromDisk(Physical_Mem, pageNum, &PageTable[oldPage]);
					PageTable[oldPage] = frame;//更新页表，并进行LRU置换
				}
				
			}
			//insert_into_TLB_FIFO(myTLB, pageNum, frame, &tlbindex);
			insert_into_TLB_LRU(myTLB,pageNum,frame,pagenum_counter);//更新TLB

		}
        int ind = ((unsigned char)frame*256) + offset;
		value = *(Physical_Mem + ind);
		printf("Physical Address: %d\t Value=%d\n", ind, value);//输出物理地址和其相应的值

	}

	fclose(address_file);
	printf("\ntlbhits: %d, pagefaults: %d\n", TLBhit, pageFault);//输出TLB命中次数和发生缺页次数
	pageFaultRate = (double)pageFault / (double)TotalAddr;
	tlbHitRate = (double)TLBhit / (double)TotalAddr;

	printf("pfRate: %.3lf, tlbhitRate: %.3lf\n", pageFaultRate, tlbHitRate);//输出TLB命中率和发生缺页错误率
	
	system("pause");
	return 0;
}
