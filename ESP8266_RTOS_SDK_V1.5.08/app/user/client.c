
#include "client.h"
#define os_memcpy memcpy
#define os_memset memset
#define os_strlen strlen
struct espconn user_tcp_conn;
LOCAL os_timer_t connect_timer;

void my_station_init(struct ip_addr *remote_ip,struct ip_addr *local_ip,int remote_port);
//������������

void ICACHE_FLASH_ATTR user_tcp_sent_cb(void *arg)//���巢��
{
	os_printf("�������ݳɹ���");
}

void ICACHE_FLASH_ATTR user_tcp_discon_cb(void *arg)//�������
{
	os_printf("�Ͽ����ӳɹ���");
}

void ICACHE_FLASH_ATTR user_tcp_recv_cb(void *arg,
		char *pdata,
		unsigned short len){
	os_printf("�յ����ݣ�%s\r\n",pdata);
	os_delay_us(300);//��ʱ
	espconn_disconnect((struct espconn *)arg);//״̬����

}

void ICACHE_FLASH_ATTR user_tcp_recon_cb(void *arg, sint8 err)//�������ӻص�����
{
	os_printf("���Ӵ��󣬴������Ϊ%d\r\n",err);
	espconn_connect((struct espconn *)arg);
}

void ICACHE_FLASH_ATTR user_tcp_connect_cb(void *arg)
{
	struct espconn *pespconn=arg;
	espconn_regist_recvcb(pespconn,user_tcp_recv_cb);//���ܳɹ��ص�
	 espconn_regist_sentcb(pespconn,user_tcp_sent_cb);//���ͳɹ� �ص�
	 espconn_regist_disconcb(pespconn,user_tcp_discon_cb);//�����ص�
	 espconn_sent(pespconn,"����esp8266",strlen("����esp8266"));
}

void ICACHE_FLASH_ATTR my_station_init(struct ip_addr *remote_ip,struct ip_addr *local_ip,int remote_port){
	//espconn�������� ��������������
	user_tcp_conn.type=ESPCONN_TCP;//����tcp
	user_tcp_conn.state=ESPCONN_NONE;//״̬
	user_tcp_conn.proto.tcp=(esp_tcp *)os_zalloc(sizeof(esp_tcp));//��ͬ������ڴ�ռ䣬os_zalloc��mem.h����
	os_memcpy(user_tcp_conn.proto.tcp->local_ip,local_ip,4);//����ip
	os_memcpy(user_tcp_conn.proto.tcp->remote_ip,remote_ip,4);//Զ��ip
	user_tcp_conn.proto.tcp->local_port=espconn_port();//ָ���˿�
	user_tcp_conn.proto.tcp->remote_port=remote_port;//Զ�̶˿�
	//ע�����ӻص������������ص�����
	espconn_regist_connectcb(&user_tcp_conn,user_tcp_connect_cb);
	espconn_regist_reconcb(&user_tcp_conn,user_tcp_recon_cb);
	//��������
	espconn_connect(&user_tcp_conn);
}


void ICACHE_FLASH_ATTR Wifi_conned(void *arg)
//ICACHE_FLASH_ATTR �Ǻ궨�岻Ҫ��gpio��uart�жϴ����лص��������쳣��
//����ص�����Wifi_conned
{
	static uint8 count=0;//������
	uint8 status;//������
	os_timer_disarm(&connect_timer);//�رն�ʱ��,connect_timerʹ�ܶ�ʱ��
	count++;
	status=wifi_station_get_connect_status();//��ѯ���ӵ�״̬�������
	if(status==STATION_GOT_IP)//�ж�����״̬
	{
		os_printf("Wifi connect success!");//������ӳɹ�
		struct ip_info info;//����ṹ��ָ��
		const char remote_ip[4]={192,168,188,187};//Զ�̵�ip��ַ������·���������� ��,�Լ��޸�
		wifi_get_ip_info(STATION_IF,&info);//��ȡ���ص�ip��ַ
		my_station_init((struct ip_addr *)remote_ip,&info.ip,8080);
		//��ʼ���ͻ�������  Զ��8080�˿�
		return;
		}else{
			if(count>=7)//�ж����Ӵ���
			{
				os_printf("Wifi connect fail��");//�������ʧ��
				return;
			}
		}
	os_timer_arm(&connect_timer,2000,NULL);
}

void ICACHE_FLASH_ATTR scan_done(void *arg,STATUS status)
//��Ӻ궨��ص�����scan_done����ָ���״̬
{
	uint8 ssid[33];
	 ESP_DBG((" 333 "));
	  if (status == OK)
	  {
	    struct bss_info *bss_link = (struct bss_info *)arg;
	    bss_link = bss_link->next.stqe_next;//ignore first
	    ESP_DBG((" 111 "));
	    while (bss_link != NULL)
	    {
	    	ESP_DBG((" 222 "));
	      os_memset(ssid, 0, 33);
	      if (os_strlen(bss_link->ssid) <= 32)
	      {
	        os_memcpy(ssid, bss_link->ssid, os_strlen(bss_link->ssid));
	      }
	      else
	      {
	        os_memcpy(ssid, bss_link->ssid, 32);
	      }
	      os_printf("+CWLAP:(%d,\"%s\",%d,\""MACSTR"\",%d)\r\n",bss_link->authmode, ssid, bss_link->rssi,MAC2STR(bss_link->bssid),bss_link->channel);
	      //�������wifi��Ϣ��������AT��Դ���н�����ֲ��ssid·�����ʺ� rssi�ź�ǿ��
          //bssid AP��mac��ַ   channel  AP��ͨ�� ��wifi��14��ͨ��
	      bss_link = bss_link->next.stqe_next;
	    }//���AP����Ϣ
	    struct station_config stationConf;//����ṹ��stationConf
	    os_memcpy(&stationConf.ssid, "Wulian_101E1D", 32);//���ӵ�·����
	    os_memcpy(&stationConf.password, "12345678", 64);//����
	    wifi_station_set_config_current(&stationConf);//wifi�����ӵĽӿ����á�sdk������
	    wifi_station_connect();//wifi���Ӻ���
	    os_timer_setfn(&connect_timer,Wifi_conned,NULL);//��ʱ���ص�����
	    os_timer_arm(&connect_timer,2000,NULL);//��ʱ��2sɨ��һ��
	  }
	  else
	  {

	  }

}

void to_scan(void)
{
    wifi_station_scan(NULL,scan_done);
}
//����ص�����to_scan����ȡ���е�ap ��ɨ����ɺ�Ļص�scan_done
