/*#############################################################################
 * �ļ�����imageenhance.cpp
 * ���ܣ�  ʵ����ͼ����ǿ�㷨
#############################################################################*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "imagemanip.h"


/******************************************************************************
** ͼ����ǿ����
**
** ����ǿ�㷨���ָ��ͼ����ƣ��������ָ��ͼ����û��ʹ�õ����򣬶�����������
** ����ǿ�󣬼��߿��Ա������ķ��������ʹ��һ����ֵ����
**
** ���㷨������һ�����߷���ͼ��һ������ͼ��
**
** �ɲο�������ƪ���£�
** 1 - Fingerprint Enhancement: Lin Hong, Anil Jain, Sharathcha Pankanti,
**     and Ruud Bolle. [Hong96]
** 2 - Fingerprint Image Enhancement, Algorithm and Performance Evaluation:
**     Lin Hong, Yifei Wan and Anil Jain. [Hong98]
**
** ��ǿ�㷨ʹ���� ����(2) �еļ������裺
**  A - ��һ��
**  B - ���㷽��ͼ
**  C - ����Ƶ��
**  D - ������������
**  E - �˲�
**
******************************************************************************/

#define P(x,y)      ((int32_t)p[(x)+(y)*pitch])

/******************************************************************************
** ������Gabor�����˲��������£�
**
**                    / 1|x'     y'  |\
** h(x,y:phi,f) = exp|- -|--- + ---| |.cos(2.PI.f.x')
**                    \ 2|dx     dy  |/
**
** x' =  x.cos(phi) + y.sin(phi)
** y' = -x.sin(phi) + y.cos(phi)
**
** �������£�
**  G ��һ�����ͼ��
**  O ����ͼ
**  F Ƶ��ͼ
**  R ����ͼ��
**  E ��ǿ���ͼ��
**  Wg Gabor�˲������ڴ�С
**
**          / 255                                          if R(i,j) = 0
**         |
**         |  Wg/2    Wg/2
**         |  ---     ---
** E(i,j)= |  \       \
**         |   --      --  h(u,v:O(i,j),F(i,j)).G(i-u,j-v) otherwise
**         |  /       /
**          \ ---     ---
**            u=-Wg/2 v=-Wg/2
**
******************************************************************************/
FvsFloat_t EnhanceGabor(FvsFloat_t x, FvsFloat_t y, FvsFloat_t phi,
                        FvsFloat_t f, FvsFloat_t r2) {
    FvsFloat_t dy2 = 1.0 / r2;
    FvsFloat_t dx2 = 1.0 / r2;
    FvsFloat_t x2, y2;
//    phi += M_PI/2;
//    x2 = -x*sin(phi) + y*cos(phi);
//    y2 =  x*cos(phi) + y*sin(phi);
    x2 = x * cos(phi) + y * sin(phi);
    y2 = - x * sin(phi) + y * cos(phi);
    return exp(-0.5 * (x2 * x2 * dx2 + y2 * y2 * dy2)) * cos(2 * M_PI * x2 * f);
}

static FvsError_t ImageEnhanceFilter
(
    FvsImage_t        normalized,
    const FvsImage_t  mask,
    const FvsFloat_t* orientation,
    const FvsFloat_t* frequence,
    FvsFloat_t        radius
) {
    FvsInt_t Wg2 = 8;
    FvsInt_t i, j, u, v;
    FvsError_t nRet  = FvsOK;
    FvsImage_t enhanced = NULL;
    FvsInt_t w        = ImageGetWidth (normalized);
    FvsInt_t h        = ImageGetHeight(normalized);
    FvsInt_t pitchG   = ImageGetPitch (normalized);
    FvsByte_t* pG     = ImageGetBuffer(normalized);
    FvsFloat_t sum, f, o;
    FvsFloat_t x2, y2, cosdir, sindir;
    /* ƽ�� */
    radius = radius * radius;
    enhanced = ImageCreate();
    if (enhanced == NULL || pG == NULL)
        return FvsMemory;
    if (nRet == FvsOK)
        nRet = ImageSetSize(enhanced, w, h);
    if (nRet == FvsOK) {
        FvsInt_t pitchE  = ImageGetPitch (enhanced);
        FvsByte_t* pE    = ImageGetBuffer(enhanced);
        if (pE == NULL)
            return FvsMemory;
        (void)ImageClear(enhanced);
        for (j = Wg2; j < h - Wg2; j++)
            for (i = Wg2; i < w - Wg2; i++) {
                if (mask == NULL || ImageGetPixel(mask, i, j) != 0) {
                    sum = 0.0;
                    o = orientation[i + j * w];
                    f = frequence[i + j * w];
                    cosdir = cos(o);
                    sindir = sin(o);
                    /*				if(i<140 && i>120 )
                                                            sum=0.0;
                                                    if(j<135 && j>120 && i==130)
                                                            sum=0.0;
                    */
                    for (v = -Wg2; v <= Wg2; v++)
                        for (u = -Wg2; u <= Wg2; u++) {
                            /*sum += EnhanceGabor
                                                        (
                                                        (FvsFloat_t)u,
                                                                        (FvsFloat_t)v,
                                                                        o,f,radius
                                                                )
                                                                * pG[(i+u)+(j+v)*pitchG];*/
                            x2 = u * cosdir + v * sindir;
                            y2 = - u * sindir + v * cosdir;
                            sum += exp(-0.5 * (x2 * x2 / radius + y2 * y2 / radius)) * cos(2 * M_PI * x2 * f) * pG[(i + u) + (j + v) * pitchG];
                        }
                    if (sum > 255.0)
                        sum = 255.0;
                    if (sum < 0.0)
                        sum = 0.0;
                    pE[i + j * pitchE] = (uint8_t)sum;
                }
            }
        nRet = ImageCopy(normalized, enhanced);
    }
    (void)ImageDestroy(enhanced);
    return nRet;
}


/******************************************************************************
** ������Gabor�����˲��������£�
**	���ݴ��ڲ��÷��ϼ��߷����16*16��
**
******************************************************************************/
static FvsError_t ImageEnhanceFilter1
(
    FvsImage_t        normalized,
    const FvsImage_t  mask,
    const FvsFloat_t* orientation,
    const FvsFloat_t* frequence,
    FvsFloat_t        radius
) {
    FvsInt_t Wg2 = 8;
    FvsInt_t i, j, u, v, x, y, x1, y1;
    FvsError_t nRet  = FvsOK;
    FvsImage_t enhanced = NULL;
    FvsInt_t w        = ImageGetWidth (normalized);
    FvsInt_t h        = ImageGetHeight(normalized);
    FvsInt_t pitchG   = ImageGetPitch (normalized);
    FvsByte_t* pG     = ImageGetBuffer(normalized);
    FvsFloat_t sum, f, o, o1, o2, rate = 1.0;
    FvsFloat_t cosdir, sindir, x2, y2;
    /* ƽ�� */
    radius = radius * radius;
    enhanced = ImageCreate();
    if (enhanced == NULL || pG == NULL)
        return FvsMemory;
    if (nRet == FvsOK)
        nRet = ImageSetSize(enhanced, w, h);
    if (nRet == FvsOK) {
        FvsInt_t pitchE  = ImageGetPitch (enhanced);
        FvsByte_t* pE    = ImageGetBuffer(enhanced);
        if (pE == NULL)
            return FvsMemory;
        (void)ImageClear(enhanced);
        for (j = Wg2; j < h - Wg2; j++)
            for (i = Wg2; i < w - Wg2; i++) {
                if (mask == NULL || ImageGetPixel(mask, i, j) != 0) {
                    sum = 0.0;
                    o = orientation[i + j * w];
                    f = frequence[i + j * w];
                    cosdir = cos(o);
                    sindir = sin(o);
                    x = (FvsInt_t)(-Wg2 / 2 * sindir) + i;
                    y = (FvsInt_t)(Wg2 / 2 * cosdir) + j;
                    o1 = frequence[x + y * w];
                    x = (FvsInt_t)(Wg2 / 2 * sindir) + i;
                    y = (FvsInt_t)(-Wg2 / 2 * cosdir) + j;
                    o2 = frequence[x + y * w];
                    /*				if(i>68 && i<139 && j>43 && j<175 && (1.0-4*fabs(o1-o2))<rate)
                                                    {
                                                            rate=1.0-4*fabs(o1-o2);
                                                            x1=i;
                                                            y1=j;
                                                    }*/
                    rate = fabs(1.0 - 10 * fabs(o1 - o2));
//				if(i<140 && i>120 )
//					sum=0.0;
//				if(j<73 && j>70 && i==130)
//					sum=0.0;
                    for (v = -Wg2; v <= Wg2; v++)
                        for (u = -Wg2; u <= Wg2; u++) {
                            x2 = u * cosdir + v * sindir;
                            y2 = - u * sindir + v * cosdir;
                            sum += exp(-0.5 * (x2 * x2 / rate / radius + y2 * y2 / rate / radius)) \
                                   *cos(2 * M_PI * x2 * f) * pG[(i + u) + (j + v) * pitchG];
                        }
                    if (sum > 255.0)
                        sum = 255.0;
                    if (sum < 0.0)
                        sum = 0.0;
                    pE[i + j * pitchE] = (uint8_t)sum;
                }
            }
        nRet = ImageCopy(normalized, enhanced);
    }
    (void)ImageDestroy(enhanced);
    return nRet;
}


static FvsError_t ImageEnhanceFilter2
(
    FvsImage_t        normalized,
    const FvsImage_t  mask,
    const FvsFloat_t* orientation,
    const FvsFloat_t* frequence,
    FvsFloat_t        radius
) {
    FvsInt_t Wg2 = 8;
    FvsInt_t i, j, u, v;
    FvsError_t nRet  = FvsOK;
    FvsImage_t enhanced = NULL;
    FvsInt_t w        = ImageGetWidth (normalized);
    FvsInt_t h        = ImageGetHeight(normalized);
    FvsInt_t pitchG   = ImageGetPitch (normalized);
    FvsByte_t* pG     = ImageGetBuffer(normalized);
    FvsFloat_t sum, f, o;
    FvsFloat_t x2, y2, cosdir, sindir;
    FvsFloat_t expv[17][17];
    radius = radius * radius;
    for (v = -Wg2; v <= Wg2; v++)
        for (u = -Wg2; u <= Wg2; u++) {
            expv[8 + v][8 + u] = exp(-0.5 * (u * u + v * v) / radius);
        }
    enhanced = ImageCreate();
    if (enhanced == NULL || pG == NULL)
        return FvsMemory;
    if (nRet == FvsOK)
        nRet = ImageSetSize(enhanced, w, h);
    if (nRet == FvsOK) {
        FvsInt_t pitchE  = ImageGetPitch (enhanced);
        FvsByte_t* pE    = ImageGetBuffer(enhanced);
        if (pE == NULL)
            return FvsMemory;
        (void)ImageClear(enhanced);
        for (j = Wg2; j < h - Wg2; j++)
            for (i = Wg2; i < w - Wg2; i++) {
                if (mask == NULL || ImageGetPixel(mask, i, j) != 0) {
                    sum = 0.0;
                    o = orientation[i + j * w];
                    f = frequence[i + j * w];
                    cosdir = cos(o);
                    sindir = sin(o);
                    if(i < 140 && i > 120 && j == 85 )
                        sum = 0.0;
                    if(j < 128 && j > 120 && i == 162)
                        sum = 0.0;
                    for (v = -Wg2; v <= Wg2; v++)
                        for (u = -Wg2; u <= Wg2; u++) {
                            x2 = u * cosdir + v * sindir;
//    y2 = - u*sindir + v*cosdir;
                            sum += expv[8 + v][8 + u] * cos(2 * M_PI * x2 * f) * pG[(i + u) + (j + v) * pitchG];
                        }
                    if (sum > 255.0)
                        sum = 255.0;
                    if (sum < 0.0)
                        sum = 0.0;
                    pE[i + j * pitchE] = (uint8_t)sum;
                }
                else	pE[i + j * pitchE] = 255;
            }
        nRet = ImageCopy(normalized, enhanced);
    }
    (void)ImageDestroy(enhanced);
    return nRet;
}



/******************************************************************************
  * ���ܣ�ָ��ͼ����ǿ�㷨
  *       ���㷨���������Ƚϸ��ӣ������Ĳ����ǻ���Gabor�˲����ģ�
          ������̬���㡣ͼ����ʱ�������θı䣬����Ҫ��һ��ԭͼ�ı��ݡ�
  * ������image        ָ��ͼ��
  *       direction    ���߷�����Ҫ���ȼ���
  *       frequency    ����Ƶ�ʣ���Ҫ���ȼ���
  *       mask         ָʾָ�Ƶ���Ч����
  *       radius       �˲����뾶�����������£�4.0���ɡ�
                       ֵԽ�����������ܵ��������ƣ�������������α������
  * ���أ�������
******************************************************************************/
FvsError_t ImageEnhanceGabor(FvsImage_t image, const FvsFloatField_t direction,
                             const FvsFloatField_t frequency, const FvsImage_t mask,
                             const FvsFloat_t radius) {
    FvsError_t nRet = FvsOK;
    FvsFloat_t * image_orientation = FloatFieldGetBuffer(direction);
    FvsFloat_t * image_frequence   = FloatFieldGetBuffer(frequency);
    if (image_orientation == NULL || image_frequence == NULL)
        return FvsMemory;
    nRet = ImageEnhanceFilter2(image, mask, image_orientation,
                               image_frequence, radius);
    return nRet;
}

