#include "stm32f10x.h"                  // Device header
#include "iic.h"
#include "delay.h"

static void IIC_SDA_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = IIC_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(IIC_SDA_PORT, &GPIO_InitStructure);
}
static void IIC_SDA_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = IIC_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(IIC_SDA_PORT, &GPIO_InitStructure);
}

/*IIC为什么要用开漏输出和上拉电阻？
    1.IIC协议支持多个主设备与多个从设备在一条总线上，如果不用开漏输出，而用推挽输出的话，会出现主设备之间短路的情况
    2.需要上拉电阻的原因是：IIC通信需要输出高电平的能力。
*/

//IIC初始化函数
void IIC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitStructure.GPIO_Pin = IIC_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;//开漏输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(IIC_SDA_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = IIC_SCL_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(IIC_SCL_PORT, &GPIO_InitStructure);

    IIC_SDA_HIGH();
    IIC_SCL_HIGH();
    Delay_us(5);   // tBUF，保证总线空闲时间

}

/*IIC开始信号
起始信号：SCL=1，SDA=1，表示开始一个新的IIC通信。在这个状态（SCL高电平，SDA高电平）下，SDA拉低，即代表准备发送数据。
*/
void IIC_Start(void)
{
    IIC_SDA_OUT();//配置SDA为输出模式，使其能够发送信号

    IIC_SDA_HIGH();//拉高SDA
    IIC_SCL_HIGH();//拉高SCL
    Delay_us(4);
    IIC_SDA_LOW();//拉低SDA
    Delay_us(4);
    IIC_SCL_LOW();//拉低SCL
}

/*IIC停止信号
停止信号：SCL=1，SDA=0，表示IIC通信结束。在这个状态（SCL高电平，SDA低电平）下，SDA拉高，即代表准备接收数据 */
void IIC_Stop(void)
{
    IIC_SDA_OUT();//SDA配置为输出模式

    IIC_SCL_LOW();//先拉低SCL
    IIC_SDA_LOW();//再拉低SDA
    Delay_us(4);
    IIC_SCL_HIGH();//拉高SCL
    Delay_us(4);
    IIC_SDA_HIGH();//SDA上升沿，产生停止信号
    Delay_us(4);
}
/*IIC等待应答信号*/
uint8_t IIC_Wait_Ack(void)
{
    uint8_t timeout = 0;

    IIC_SDA_IN();//SDA配置为输入模式
    IIC_SDA_HIGH();//拉高SDA
    Delay_us(2);
    IIC_SCL_HIGH();//拉高SCL
    Delay_us(2);
    while(IIC_SDA_READ() == 1)
    {
        timeout++;
        if(timeout > 250)//超时处理
        {
            IIC_Stop();//产生停止信号
            return 1;//返回失败
        }   
    }
    Delay_us(2);
    IIC_SCL_LOW();//拉低SCL,结束应答时钟
    return 0;//返回成功              
}
/*主机产生应答信号*/
void IIC_Ack(void)
{
    IIC_SDA_OUT();//SDA配置为输出模式

    IIC_SCL_LOW();//先拉低SCL
    IIC_SDA_LOW();//拉低SDA，表示应答
    Delay_us(2);

    IIC_SCL_HIGH();//拉高SCL
    Delay_us(2);
    IIC_SCL_LOW();//拉低SCL
}
/*主机产生非应答信号*/
void IIC_NAck(void)
{
    IIC_SDA_OUT();//SDA配置为输出模式

    IIC_SCL_LOW();//先拉低SCL
    Delay_us(2);

    IIC_SDA_HIGH();//拉高SDA,表示非应答
    Delay_us(2);

    IIC_SCL_HIGH();//拉高SCL
    Delay_us(2);
    IIC_SCL_LOW();//拉低SCL
}
/*IIC发送一个字节*/
void IIC_Write_Byte(uint8_t data)
{
    uint8_t i;

    IIC_SDA_OUT();//SDA配置为输出模式
    IIC_SCL_LOW();//先拉低SCL
    for(i = 0; i < 8; i++)
    {
        if(data & 0x80)
        {
            IIC_SDA_HIGH();
        }
        else
        {
            IIC_SDA_LOW();
        }
        data <<= 1;
        Delay_us(2);

        IIC_SCL_HIGH();
        Delay_us(2);
        IIC_SCL_LOW();
        Delay_us(2);
    }
}
/*
 * 函  数: uint8_t IIC_Read_Byte(uint8_t ack)
 * 功  能: IIC读取一个字节
 * 参  数: ack - 1:发送ACK，0:发送NACK
 * 原  理: 1. 主机释放SDA（设为输入），让从机控制SDA
 *         2. 在SCL高电平时读取SDA数据
 *         3. 高位先接收（MSB First）
 * 返回值：读取到的字节数据*/
uint8_t IIC_Read_Byte(uint8_t ack)
{
    uint8_t i, data = 0;

    IIC_SDA_IN();//SDA配置为输入模式
    for(i = 0; i < 8; i++)
    { 
        IIC_SCL_LOW();
        Delay_us(2);
        IIC_SCL_HIGH();
        data <<= 1;
        
        if (IIC_SDA_READ())
        {
            data |= 0x01;
        }
        Delay_us(2);
    }
    if(ack)
    {
         IIC_Ack();
    }
    else{
         IIC_NAck();
    }
    IIC_SCL_LOW();
    return data;
        
}
/*向IIC设备写入数据*/
uint8_t IIC_Write_Data(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data,uint16_t len)
{
    uint16_t i;

    IIC_Start();//产生起始信号

    IIC_Write_Byte((dev_addr << 1)|0);//发送设备地址+写命令

    if(IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1;
    }
    IIC_Write_Byte(reg_addr);//发送寄存器地址

    if(IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1;
    }
    for(i = 0; i < len; i++)
    {
        IIC_Write_Byte(data[i]);//发送数据
        if(IIC_Wait_Ack())
        {
            IIC_Stop();
            return 1;
        }
    }
    IIC_Stop();
    return 0;
}

/*从iic设备读取数据*/
uint8_t IIC_Read_Data(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    uint16_t i;

    IIC_Start();//发送起始信号

    IIC_Write_Byte((dev_addr << 1)|0);//发送设备地址+写命令

    if(IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1;
    }
    IIC_Write_Byte(reg_addr);//发送寄存器地址
    if(IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1;
    }
    IIC_Start();//发送起始信号

    IIC_Write_Byte((dev_addr << 1)|1);//发送设备地址+读命令

    if(IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1;
    }
    for(i = 0; i < len; i++)
    {
       if(i == (len - 1))
       {
        data[i] = IIC_Read_Byte(0);//读取最后一个字节，不需要应答
       }
       else
       {
         data[i] = IIC_Read_Byte(1);//读取其他字节，需要应答
       }
    }
    IIC_Stop();
    return 0;
}
