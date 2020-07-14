library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

source("evaluation/human_readable.R")

pingpong_net_master_full <- read.csv("evaluation/data/pingpong-timing-net.out", sep=",")
pingpong_net_master <- pingpong_net_master_full[ ,1:ncol(pingpong_net_master_full)-1]
pingpong_net_master$avg <- rowMeans(pingpong_net_master[,2:ncol(pingpong_net_master)])
pingpong_net_master$sdev <- apply(pingpong_net_master[ ,2:ncol(pingpong_net_master)], 1, sd)
pingpong_net_master$backend <- 'net'

pingpong_io_full <- read.csv("evaluation/data/pingpong-timing-io.out", sep=",")
pingpong_io <- pingpong_io_full[ ,1:ncol(pingpong_net_master_full)-1]
pingpong_io$avg <- rowMeans(pingpong_io[,2:ncol(pingpong_io)])
pingpong_io$sdev <- apply(pingpong_io[,2:ncol(pingpong_io)], 1, sd)
pingpong_io$backend <- 'io'


ppdf <- rbind(pingpong_net_master, pingpong_io)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev

print(pingpong_io$sdev)
print(pingpong_net_master$sdev)

print(ppdf$upper)
print(ppdf$lower)

pp_plot <- ggplot(ppdf, aes(x=what, y=avg, fill=backend)) + 
  scale_y_continuous(labels = human_numbers, limits=c(0,115), breaks=seq(0, 110, 10)) +
  geom_bar(stat="identity", position=position_dodge()) +
  geom_errorbar(aes(ymin=lower, ymax=upper), width=.2,
                position=position_dodge(.9))+
  ggtitle("Receiving durations in Pingpong") +
  labs(x="Interval", y="duration [µs]")
  
# tikz(file="figs/pingpong-timings.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/pingpong-timings-recv.pdf", plot=pp_plot, width=8, height=5)

