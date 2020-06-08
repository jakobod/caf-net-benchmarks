library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

source("evaluation/human_readable.R")

pingpong_net_master <- read.csv("evaluation/data/pingpong-timing-net.out", sep=",")[ ,1:50000]
pingpong_net_master$avg <- rowMeans(pingpong_net_master[,2:3])
pingpong_net_master$sdev <- apply(pingpong_net_master, 1, sd)
pingpong_net_master$proto <- '1 net - master'

pingpong_io <- read.csv("evaluation/data/pingpong-timing-io.out", sep=",")[ ,1:50000]
pingpong_io$avg <- rowMeans(pingpong_io[,2:4])
pingpong_io$sdev <- apply(pingpong_io, 1, sd)
pingpong_io$proto <- '1 io'


ppdf <- rbind(pingpong_net_master, pingpong_io)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev

pp_plot <- ggplot(ppdf, aes(x=what, y=avg, fill=proto)) + 
  scale_x_continuous(labels = human_numbers, limits=c(0.5,2.5), breaks=seq(1, 2, 1)) +
  scale_y_continuous(labels = human_numbers, limits=c(0,255), breaks=seq(0, 250, 10)) +
  geom_bar(stat="identity", position=position_dodge()) +
  geom_errorbar(aes(ymin=lower, ymax=upper), width=.2,
                position=position_dodge(.9))+
  ggtitle("Pingpong multiple remote nodes 1 Ping") +
  labs(x="", y="throughput [pongs/s]")
  
# tikz(file="figs/pingpong-timings.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/pingpong-timings.pdf", plot=pp_plot, width=8, height=5)
