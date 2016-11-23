#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void get_mod_rm_reg(unsigned char , unsigned char *,unsigned char *, unsigned char *);
static unsigned char get_disp_length(unsigned char , unsigned char );
static void print_out_registor(void);

typedef struct {
	int eax;
	int ecx;
	int edx;
	int ebx;
	int esp;
	int ebp;
	int esi;
	int edi;
} CPU_GE_REGISTOR;

typedef struct {
	int eip;
	int eflags;
} CPU_SP_RGISTER;

typedef struct {
	CPU_GE_REGISTOR g_reg;
	CPU_SP_RGISTER sp_reg;
}CPU_REG;

typedef struct {
	unsigned char code;//ex.mov 89
	unsigned char length;//byte
}_OPE_CODE;

CPU_REG cpu_reg = {0};

_OPE_CODE ope_code[] = {
	{0x89, 1},//2コメはrmmodだと想定。。。
	{0x83, 1}, 
};

unsigned char main_memory_stack[2048] = {0};
unsigned char main_memory[8] = {0};


/* immediate codeのバイト数を返す。byte */
static unsigned char get_imme_length(unsigned char code){
	if (code == 0x83) {
		return 1;
	}
}
/*メインメモリから命令を取り出す*/
static unsigned char fetch(unsigned char *mnemonic){
	unsigned char code = 0;
	unsigned char length = 0;
	unsigned char MOD;
	unsigned char RM;
	unsigned char REG;
	unsigned char disp_len;

	int eip = cpu_reg.sp_reg.eip;

	/*1byte取り出し*/
	code = *(main_memory + eip);
	length++;//1byte取り出し分

	//modrmとdispをとりだしを取り出し。
	if (code == 0x89){
		get_mod_rm_reg(*(main_memory + eip + 1), &MOD, &RM, &REG);//+1はRM/MOD
		length++;//modrm分
		disp_len = get_disp_length(MOD, RM);
		length = length + disp_len;//disp_len分
	} else if (code == 0x83){
		get_mod_rm_reg(*(main_memory + eip + 1), &MOD, &RM, &REG);//+1はRM/MOD
		length++;//modrm分
		disp_len = get_disp_length(MOD, RM);
		length = length + disp_len;//disp_len分

		/*immediate code length*/
		length = length + get_imme_length(code);
	} else if (code == 0x90) {
	} else if (code == 0x55) {
		length = 1;
	} else if (code == 0x48) {
		length = 1;
	} else if (code == 0x5d) {
		length = 1;
	}

	/* メインメモリーからプログラムを取り出し */
	memcpy(mnemonic, main_memory + eip, length);
	
	/*
	 * デバッグプリント
	 */
	int i;
	for(i = 0; i < 15;i++){
		printf("f%d(%x)", i, mnemonic[i]);
	}
	printf("\n");

	/*eipを移動*/
	cpu_reg.sp_reg.eip = cpu_reg.sp_reg.eip + length;

	return length;
}

static int *get_register_from_MOD_RM(unsigned char *mnemonic, unsigned char MOD, unsigned char RM)
{
	//displasementがあれば足す
	int disp_8bit = 0;
	int disp_32bit = 0;

	int disp_len = get_disp_length(MOD, RM);
	printf("%d\n", disp_len);

	if(disp_len == 1){
		memcpy(&disp_8bit, mnemonic + 2, 1);//1 1byte
	} else if (disp_len == 4){
		memcpy(&disp_32bit, mnemonic + 2, 4);//4 1byte
		//エンディアン変換がいるかも 
	}
	printf("disp32 = %d\n", disp_32bit);
	printf("disp8 = %d\n", disp_8bit);

	int *ret = NULL;

	/*
	 * Mod
	 * 00 レジスタに入ってるアドレスにかいこむ
	 * 01 アドレス＋disp8
	 * 10 アドレス＋disp32
	 * 11 レジスタに書き込む
	 */
	if (MOD == 0) {//00
		switch (RM) {
			case 0x0:
				ret = (int*)cpu_reg.g_reg.eax;
				break;
			case 0x1://001
				ret = (int*)cpu_reg.g_reg.ecx;
				break;
			case 0x2://010
				ret = (int*)cpu_reg.g_reg.edx;
				break;
			case 0x3://011
				ret = (int*)cpu_reg.g_reg.ebx;
				break;
			case 0x4://100
				printf("niy %d\n", __LINE__);
				break;
			case 0x5://101
				ret = (int*)disp_32bit;
				break;
			case 0x6://110
				ret = (int*)cpu_reg.g_reg.esi;
				break;
			case 0x7://111                
				ret = (int*)cpu_reg.g_reg.edi;
				break;
			default:
				printf("error\n");
				break;
		}
	}  else if (MOD == 1) {//01
		switch (RM) {
			case 0x0://000
				ret = (int*)(cpu_reg.g_reg.eax + disp_8bit);
				break;
			case 0x1://001
				ret = (int*)(cpu_reg.g_reg.ecx + disp_8bit);
				break;
			case 0x2://010
				ret = (int*)(cpu_reg.g_reg.edx + disp_8bit);
				break;
			case 0x3://011
				ret = (int*)(cpu_reg.g_reg.ebx + disp_8bit);
				break;
			case 0x4://100
				/*[-][-]+disp32 */
				break;
			case 0x5://101
				ret = (int*)(cpu_reg.g_reg.ebp + disp_8bit);
				break;
			case 0x6://110
				ret = (int*)(cpu_reg.g_reg.esi + disp_8bit);
				break;
			case 0x7://111
				ret = (int*)(cpu_reg.g_reg.edi + disp_8bit);
				break;
			default:
				printf("error\n");
				break;
		}
	} else if (MOD == 2) {//010
		switch (RM) {
			case 0x0:
				ret = (int*)(cpu_reg.g_reg.eax + disp_32bit);
				break;
			case 0x1:
				ret = (int*)(cpu_reg.g_reg.ecx + disp_32bit);
				break;
			case 0x2:
				ret = (int*)(cpu_reg.g_reg.edx + disp_32bit);
				break;
			case 0x3:
				ret = (int*)(cpu_reg.g_reg.ebx + disp_32bit);
				break;
			case 0x4:
				/* [-][-]+disp32 */
				break;
			case 0x5:
				ret = (int*)(cpu_reg.g_reg.ebp + disp_32bit);
				break;
			case 0x6:
				ret = (int*)(cpu_reg.g_reg.esi + disp_32bit);
				break;
			case 0x7:                     
				ret = (int*)(cpu_reg.g_reg.edi + disp_32bit);
				break;
			default:
				printf("error\n");
				break;
		}
	} else if(MOD == 3){//11
		switch (RM) {
			case 0x0:
				ret = &cpu_reg.g_reg.eax;
				break;
			case 0x1:
				ret = &cpu_reg.g_reg.ecx;
				break;
			case 0x2:
				ret = &cpu_reg.g_reg.edx;
				break;
			case 0x4:
				ret = &cpu_reg.g_reg.ebx;
				break;
			case 0x3:
				ret = &cpu_reg.g_reg.esp;
				break;
			case 0x5:
				ret = &cpu_reg.g_reg.ebp;
				break;
			case 0x6:
				ret = &cpu_reg.g_reg.esi;
				break;
			case 0x7:                     
				ret = &cpu_reg.g_reg.edi;
				break;
			default:
				printf("error\n");
				break;
		}
	}
	printf("get_register_from_MOD_RM ret %x\n", (int*)ret);
	return ret;
}


static int get_register_from_REG(unsigned char REG){
	int ret = 0;
	switch (REG) {
		case 0x0:
			ret = cpu_reg.g_reg.eax;
			break;
		case 0x1:
			ret = cpu_reg.g_reg.ecx;
			break;
		case 0x2:
			ret = cpu_reg.g_reg.edx;
			break;
		case 0x3:
			ret = cpu_reg.g_reg.ebx;
			break;
		case 0x4:
			ret = cpu_reg.g_reg.esp;
			break;
		case 0x5:
			ret = cpu_reg.g_reg.ebp;
			break;
		case 0x6:
			ret = cpu_reg.g_reg.esi;
			break;
		case 0x7:                     
			ret = cpu_reg.g_reg.edi;
			break;
		default:
			printf("error\n");
			break;
	}
	printf("get_register_from_REG ret = %d\n", ret);
	return ret;
}



static void get_mod_rm_reg(unsigned char mod_opeland, unsigned char *MOD,
			unsigned char *RM, unsigned char *REG) {
	*MOD = mod_opeland >> 6;
	unsigned char RM_tmp = mod_opeland << 5;
	*RM = RM_tmp >> 5;
	unsigned char REG_mod = mod_opeland << 2;
	*REG = REG_mod >> 5;

	printf("MOD=%d\n", *MOD);
	printf("RM=%d\n", *RM);
	printf("REG=%d\n", *REG);
}
static unsigned char get_disp_length(unsigned char MOD, unsigned char RM){
	char ret = 0;
	switch (MOD){
	case 0x10:
		ret = 4;	
		break;
	case 0x01:
		ret = 1;	
		break;
	case 0x00:
		if (RM == 0x101){
			ret = 4;	
		}
		break;
	default:
		break;
	}

	printf("get_disp_length%d\n", ret);
	return ret;

}

/*命令を解読する*/
static void execute(unsigned char *mnemonic) {
	unsigned char MOD;
	unsigned char RM;
	unsigned char REG;
	int *reg_register1;
	int *reg_register2;
	char displasement_8bit;
	int displasement_32bit;
	unsigned char disp_len;

	if (*mnemonic == 0x89) {
		printf("mov 89\n");
		//opecode1byte modrm1byte disp VARbyte
		get_mod_rm_reg(*(mnemonic+1), &MOD, &RM, &REG);

		//REGは上書く側のレジスタ(後ろ) 足しこむ奴が入ってるやつの値
		reg_register2 = get_register_from_REG(REG);

		//get_register_from_MODで上書きする場所を取得
		reg_register1 = get_register_from_MOD_RM(mnemonic, MOD, RM);

		/* 足しこむ場所のアドレスがわかったので */
		printf("reg_register1 %d\n", reg_register1);
		printf("reg_register2 %d\n", reg_register2);
		*reg_register1 = reg_register2;
	} else if (*mnemonic == 0x83) {
		printf("add 83\n");
		//opecode1byte modrm1byte disp VARbyte
		int immediate = 1;
		get_mod_rm_reg(*(mnemonic+1), &MOD, &RM, &REG);
		if(MOD == 1){//0x01
			immediate = *(mnemonic+3);
		} else if (MOD == 2){//0x10
			immediate =  *(mnemonic+6);
		}
		//get_register_from_MODで上書きする場所を取得
		reg_register1 = get_register_from_MOD_RM(mnemonic, MOD, RM);

		//immediate codeを書き込み
		*reg_register1  = *reg_register1 + immediate;
	} else if (*mnemonic == 0x90) {
		printf("nop 90\n");
	} else if (*mnemonic == 0x55) {
		printf("push 55\n");
		cpu_reg.g_reg.esp  += sizeof(int);
		*(int*)(cpu_reg.g_reg.esp) = cpu_reg.g_reg.ebp;
	} else if (*mnemonic == 0x48) {
		printf("dec 48\n");
		cpu_reg.g_reg.eax--;
	} else if (*mnemonic == 0x5d) {
		printf("pop 5d\n");
		cpu_reg.g_reg.ebp = *(int*)(cpu_reg.g_reg.esp);
		cpu_reg.g_reg.esp -= sizeof(int);
	}
}

static void print_mnemonic(unsigned char *mn){
	int i;
	printf("mne=");
	for(i = 0; i < 15; i){
		printf("%x,", mn[i]);
		i++;
	}
	printf("\n");
}

/*実行*/
static void simulator_run() {
	/* ニーモニックの箱 */
	unsigned char mnemonic[15] = {0};
	int i;
	for (i = 0;i < 6; i++) {
		getchar();
		memset(mnemonic, 0, 15);
		fetch(mnemonic);
		print_mnemonic(mnemonic);
		execute(mnemonic);
		print_out_registor();
	}

}

static void print_out_registor(){
	printf("%d eax(%d)\n",&cpu_reg.g_reg.eax,cpu_reg.g_reg.eax);
	printf("%d ecx(%d)\n",&cpu_reg.g_reg.ecx,cpu_reg.g_reg.ecx);
	printf("%d edx(%d)\n",&cpu_reg.g_reg.edx,cpu_reg.g_reg.edx);
	printf("%d ebx(%d)\n",&cpu_reg.g_reg.ebx,cpu_reg.g_reg.ebx);
	printf("%d esp(%d)\n",&cpu_reg.g_reg.esp,cpu_reg.g_reg.esp);
	printf("%d ebp(%d)\n",&cpu_reg.g_reg.ebp,cpu_reg.g_reg.ebp);
	printf("%d esi(%d)\n",&cpu_reg.g_reg.esi,cpu_reg.g_reg.esi);
	printf("%d edi(%d)\n",&cpu_reg.g_reg.edi,cpu_reg.g_reg.edi);
	printf("%d eip(%d)\n",&cpu_reg.sp_reg.eip,cpu_reg.sp_reg.eip);
	printf("%d eflags(%d)\n",&cpu_reg.sp_reg.eflags,cpu_reg.sp_reg.eflags);
	printf("----\n");
}

int main() {
	/*espを適当なところに設定 グローバル領域で代入できないのでしかたなくここでやる*/
	cpu_reg.g_reg.esp = &main_memory_stack[1024];
	printf("esp initial data %d\n", cpu_reg.g_reg.esp);

	FILE *fp;
	fp = fopen("func.bin","rb");

	//ファイルfpからsizeバイトのデータをn個読み込み、bufに格納します。
	//ファイル位置指示子を読み込んだデータバイト分進めます。
	fread(main_memory,10,1,fp);
	int i = 0;
	for(i = 0; i < 8;i++){
		printf("%x\n", main_memory[i]);
	}
	fclose(fp);

	simulator_run();

	return 0;
}


