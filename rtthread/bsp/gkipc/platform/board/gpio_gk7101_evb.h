#ifndef GPIO_CFG_EVB_H
#define GPIO_CFG_EVB_H
#include "adi_types.h"
#include "adi_gpio.h"

#define SYSTEM_USE_EXTERN_I2S       1
#define SYSTEM_USE_SDCARD           1
#define SYSTEM_USE_EXTERN_ETHPHY    1
#define SYSTEM_INIT_GD_GPIO         0

#if SYSTEM_USE_EXTERN_I2S == 1
#define SYSTEM_GPIO_I2S_TABLE                              \
	{ GADI_GPIO_4,  GADI_GPIO_TYPE_OUTPUT_AOMCLK			}, \
	{ GADI_GPIO_5 , GADI_GPIO_TYPE_OUTPUT_AOLRCLK			}, \
	{ GADI_GPIO_6 , GADI_GPIO_TYPE_INPUT_I2S_SI 			}, \
	{ GADI_GPIO_7 , GADI_GPIO_TYPE_OUTPUT_AOBCLK			},
#else
#define SYSTEM_GPIO_I2S_TABLE                              \
    { GADI_GPIO_4 , GADI_GPIO_TYPE_OUTPUT_SPI0_SO           }, \
    { GADI_GPIO_5 , GADI_GPIO_TYPE_OUTPUT_SPI1_SO           }, \
    { GADI_GPIO_6 , GADI_GPIO_TYPE_OUTPUT_SPI0_SCLK         }, \
    { GADI_GPIO_7 , GADI_GPIO_TYPE_OUTPUT_PWM3_OUT          },
#endif

#if SYSTEM_USE_SDCARD == 1
#define SYSTEM_GPIO_SD_TABLE                               \
    { GADI_GPIO_40, GADI_GPIO_TYPE_INOUT_SD_DATA_3          }, \
    { GADI_GPIO_41, GADI_GPIO_TYPE_INPUT_SD_WP_N            }, \
    { GADI_GPIO_42, GADI_GPIO_TYPE_INOUT_SD_DATA_0          }, \
    { GADI_GPIO_43, GADI_GPIO_TYPE_INOUT_SD_DATA_2          }, \
    { GADI_GPIO_44, GADI_GPIO_TYPE_INOUT_SD_DATA_1          }, \
    { GADI_GPIO_45, GADI_GPIO_TYPE_UNDEFINED                }, \
    { GADI_GPIO_46, GADI_GPIO_TYPE_OUTPUT_SDIO_CLK          }, \
    { GADI_GPIO_47, GADI_GPIO_TYPE_UNDEFINED                }, \
    { GADI_GPIO_48, GADI_GPIO_TYPE_INOUT_SD_CMD             }, \
    { GADI_GPIO_49, GADI_GPIO_TYPE_UNDEFINED                }, \
    { GADI_GPIO_50, GADI_GPIO_TYPE_INPUT_SD_CD_N            },
#else
#define SYSTEM_GPIO_SD_TABLE                               \
    { GADI_GPIO_40, GADI_GPIO_TYPE_OUTPUT_AHB_DAC_DR_9      }, \
    { GADI_GPIO_41, GADI_GPIO_TYPE_OUTPUT_1 /* USB_PD */    }, \
    { GADI_GPIO_42, GADI_GPIO_TYPE_OUTPUT_AHB_DAC_DR_6      }, \
    { GADI_GPIO_43, GADI_GPIO_TYPE_OUTPUT_AHB_DAC_DR_7      }, \
    { GADI_GPIO_44, GADI_GPIO_TYPE_OUTPUT_AHB_DAC_DR_8      }, \
    { GADI_GPIO_45, GADI_GPIO_TYPE_OUTPUT_AHB_DAC_DR_3      }, \
    { GADI_GPIO_46, GADI_GPIO_TYPE_OUTPUT_AHB_DAC_DR_4      }, \
    { GADI_GPIO_47, GADI_GPIO_TYPE_OUTPUT_AHB_DAC_DR_5      }, \
    { GADI_GPIO_48, GADI_GPIO_TYPE_OUTPUT_AHB_DAC_DR_0      }, \
    { GADI_GPIO_49, GADI_GPIO_TYPE_OUTPUT_AHB_DAC_DR_1      }, \
    { GADI_GPIO_50, GADI_GPIO_TYPE_OUTPUT_AHB_DAC_DR_2      },
#endif

#if SYSTEM_USE_EXTERN_ETHPHY == 1
#define SYSTEM_GPIO_PHY1_TABLE                             \
    { GADI_GPIO_18, GADI_GPIO_TYPE_INPUT_ENET_PHY_RXDV      }, \
    { GADI_GPIO_19, GADI_GPIO_TYPE_INPUT_ENET_PHY_RXER      },
#define SYSTEM_GPIO_PHY2_TABLE                             \
    { GADI_GPIO_31, GADI_GPIO_TYPE_INPUT_ENET_PHY_RXD_1     }, \
    { GADI_GPIO_32, GADI_GPIO_TYPE_OUTPUT_AOMCLK            }, \
    { GADI_GPIO_33, GADI_GPIO_TYPE_OUTPUT_ENET_PHY_RESET /*ETH reset*/   }, \
    { GADI_GPIO_34, GADI_GPIO_TYPE_OUTPUT_ENET_PHY_TXEN     }, \
    { GADI_GPIO_35, GADI_GPIO_TYPE_OUTPUT_ENET_GMII_MDC_O   }, \
    { GADI_GPIO_36, GADI_GPIO_TYPE_INOUT_ETH_MDIO           },
#define SYSTEM_GPIO_PHY3_TABLE                             \
    { GADI_GPIO_53, GADI_GPIO_TYPE_INPUT_ENET_PHY_RXD_0     }, \
    { GADI_GPIO_54, GADI_GPIO_TYPE_INPUT_ENET_PHY_RXD_2/*CLK*/},
#else
#define SYSTEM_GPIO_PHY1_TABLE                             \
    { GADI_GPIO_18, GADI_GPIO_TYPE_OUTPUT_EPHY_LED_3        }, \
    { GADI_GPIO_19, GADI_GPIO_TYPE_OUTPUT_EPHY_LED_1        },
#define SYSTEM_GPIO_PHY2_TABLE                             \
    { GADI_GPIO_31, GADI_GPIO_TYPE_UNDEFINED                }, \
    { GADI_GPIO_32, GADI_GPIO_TYPE_OUTPUT_AOMCLK            }, \
    { GADI_GPIO_33, GADI_GPIO_TYPE_UNDEFINED                }, \
    { GADI_GPIO_34, GADI_GPIO_TYPE_UNDEFINED                }, \
    { GADI_GPIO_35, GADI_GPIO_TYPE_UNDEFINED                }, \
    { GADI_GPIO_36, GADI_GPIO_TYPE_UNDEFINED                },
#define SYSTEM_GPIO_PHY3_TABLE                             \
    { GADI_GPIO_53, GADI_GPIO_TYPE_UNDEFINED                }, \
    { GADI_GPIO_54, GADI_GPIO_TYPE_UNDEFINED                },
#endif


#define SYSTEM_GPIO_XREF_TABLE                             \
    { GADI_GPIO_0 , GADI_GPIO_TYPE_OUTPUT_SF_CS0            }, \
    { GADI_GPIO_1 , GADI_GPIO_TYPE_OUTPUT_SF_CS1            }, \
    { GADI_GPIO_2 , GADI_GPIO_TYPE_OUTPUT_ENET_PHY_TXD_1    }, \
    { GADI_GPIO_3 , GADI_GPIO_TYPE_OUTPUT_ENET_PHY_TXD_0    }, \
	SYSTEM_GPIO_I2S_TABLE\
    { GADI_GPIO_8 , GADI_GPIO_TYPE_OUTPUT_1 /* SPI0_HOLD */ }, \
    { GADI_GPIO_9 , GADI_GPIO_TYPE_OUTPUT_PWM2_OUT          }, \
    { GADI_GPIO_10, GADI_GPIO_TYPE_OUTPUT_PWM1_OUT          }, \
    { GADI_GPIO_11, GADI_GPIO_TYPE_OUTPUT_SPI0_CS0          }, \
    { GADI_GPIO_12, GADI_GPIO_TYPE_OUTPUT_PWM0_OUT          }, \
    { GADI_GPIO_13, GADI_GPIO_TYPE_INPUT_SPI1_SI            }, \
    { GADI_GPIO_14, GADI_GPIO_TYPE_INPUT_SPI0_SI            }, \
    { GADI_GPIO_15, GADI_GPIO_TYPE_OUTPUT_1 /* SPI0_WP */   }, \
    { GADI_GPIO_16, GADI_GPIO_TYPE_OUTPUT_SPI1_CS0          }, \
    { GADI_GPIO_17, GADI_GPIO_TYPE_OUTPUT_SPI1_SCLK         }, \
    SYSTEM_GPIO_PHY1_TABLE\
    { GADI_GPIO_20, GADI_GPIO_TYPE_INPUT_UART0_RX           }, \
    { GADI_GPIO_21, GADI_GPIO_TYPE_OUTPUT_UART2_DTR_N/* RS485_RE */}, \
    { GADI_GPIO_22, GADI_GPIO_TYPE_OUTPUT_UART0_TX          }, \
    { GADI_GPIO_23, GADI_GPIO_TYPE_OUTPUT_UART1_TX          }, \
    { GADI_GPIO_24, GADI_GPIO_TYPE_OUTPUT_UART2_TX          }, \
    { GADI_GPIO_25, GADI_GPIO_TYPE_INPUT_UART1_RX           }, \
    { GADI_GPIO_26, GADI_GPIO_TYPE_INPUT_UART2_RX           }, \
    { GADI_GPIO_27, GADI_GPIO_TYPE_OUTPUT_1 /*Sensor reset*/}, \
    { GADI_GPIO_28, GADI_GPIO_TYPE_INOUT_I2C_DATA /*S D*/   }, \
    { GADI_GPIO_29, GADI_GPIO_TYPE_INOUT_I2C_CLK  /*S C*/   }, \
    { GADI_GPIO_30, GADI_GPIO_TYPE_OUTPUT_AO_DATA0          }, \
    SYSTEM_GPIO_PHY2_TABLE\
    { GADI_GPIO_37, GADI_GPIO_TYPE_OUTPUT_1 /*IR_CUT*/      }, \
    { GADI_GPIO_38, GADI_GPIO_TYPE_INOUT_I2C_DATA2          }, \
    { GADI_GPIO_39, GADI_GPIO_TYPE_INOUT_I2C_CLK2           }, \
    SYSTEM_GPIO_SD_TABLE\
    { GADI_GPIO_51, GADI_GPIO_TYPE_OUTPUT_AOBCLK            }, \
    { GADI_GPIO_52, GADI_GPIO_TYPE_OUTPUT_AOLRCLK           }, \
    SYSTEM_GPIO_PHY3_TABLE\
    { GADI_GPIO_55, GADI_GPIO_TYPE_UNDEFINED                }


#define SYSTEM_GPIO_IR_CUT1                         GADI_GPIO_10
#define SYSTEM_GPIO_IR_CUT2                         GADI_GPIO_9
#define SYSTEM_ETH_PHY_RESET_GPIO                   GADI_GPIO_33
#define SYSTEM_GPIO_SENSOR_RESET    				GADI_GPIO_27
#define SYSTEM_GPIO_USB_HOST                        GADI_GPIO_49

#endif
