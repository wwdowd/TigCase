#1. create one directory per Arduino
#2. why ggplot2 not working on laptop?
#3. check time zone issue? 

setwd("D:/My_documents/WSU/Experiments/Tigriopus/R code/pH DO test") 
#set working directory to the file that contains all the .csv files for that water source.

filenames <- choose.files(default = "", caption = "Select 1 or more TigBathXX files",multi = TRUE)#file.choose()

#LINE BELOW WILL EXTRACT ALL CSV FILES
#filenames <- list.files(full.names=TRUE)
All <- lapply(filenames,function(i){
  read.csv(i,header=TRUE)
})
df <- do.call(rbind.data.frame, All)

#sort input data by rows by unixtime, in case filenaming gets out of order
df <-df[order(df$unixtime),]

#previous attempt didn't work
#df$Date <-as.POSIXct(df$date_time)
df$Date <- as.POSIXct(df$unixtime, origin="1970-01-01",tz="UTC") #"America/Los_Angeles")

#install.packages("ggplot2")
#install.packages("gridExtra")

library(ggplot2)
library(gridExtra)

pH <- ggplot()+
  #data = df, aes(y = pH,x =Date))+
  #geom_point()+
  geom_line(data = df, aes(x = Date, y = pH), size = 1.5, color = "black") +
  geom_line(data = df, aes(x = Date, y = pHset), color = "grey") +
  theme_bw()+ 
  ylab("pH")+
  xlab("Date (MM DD)")+
  ggtitle("pH, black = observed, grey = setpoint")

O2 <- ggplot()+
  #data = df, aes(y = O2_mgL,x =Date))+
  #geom_point()+
  geom_line(data = df, aes(x = Date, y = O2_mgL), size=1.5, color = "black") +
  geom_line(data = df, aes(x = Date, y = DOset), color = "grey")+
  theme_bw()+ 
  ylab("O2 mg/L")+
  xlab("Date (MM DD)")+
  ggtitle("Oxygen, black = observed, grey = setpoint")

grid.arrange(pH, O2, ncol=2, nrow =1)
