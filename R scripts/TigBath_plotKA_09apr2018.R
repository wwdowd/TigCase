#setwd("C:/dowdlabscripts")

df1 <- read.csv("TigBath01_record.csv", header = F)
df2 <- read.csv("TigBath02_record.csv", header = F)

df1$Date <-as.POSIXct(df1$V1)
df2$Date <-as.POSIXct(df2$V1)

#install.packages("ggplot2")
#install.packages("gridExtra")

library(ggplot2)
library(gridExtra)

p1 <- ggplot(data = df1, aes(y = V3,x =Date))+
  geom_point()+
  theme_bw()+ 
  ylab("")+
  xlab("Date (MM DD)")+
  ggtitle("Bath 1")

p2 <- ggplot(data = df2, aes(y = V3,x =Date))+
  geom_point()+
  theme_bw()+ 
  ylab("")+
  xlab("Date (MM DD)")+
  ggtitle("Bath 2")

grid.arrange(p1, p2, ncol=4, nrow =2)
