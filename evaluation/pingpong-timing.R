library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

source("evaluation/human_readable.R")

pingpong_net_master <- read.csv("evaluation/data/pingpong-timing-net.out", sep=",")[ ,1:40000]
pingpong_net_master$avg <- rowMeans(pingpong_net_master[,2:40000])
pingpong_net_master$sdev <- apply(pingpong_net_master[ ,2:40000], 1, sd)
pingpong_net_master$backend <- 'net'

pingpong_fix <- read.csv("evaluation/data/pingpong-timing-fix-net.out", sep=",")[ ,1:40000]
pingpong_fix$avg <- rowMeans(pingpong_fix[,2:40000])
pingpong_fix$sdev <- apply(pingpong_fix[ ,2:40000], 1, sd)
pingpong_fix$backend <- 'net - fixed'


ppdf <- rbind(pingpong_net_master, pingpong_fix)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev

print(pingpong_io$sdev)
print(pingpong_net_master$sdev)

print(ppdf$upper)
print(ppdf$lower)

pp_plot <- ggplot(ppdf, aes(x=what, y=avg, fill=backend)) + 
  scale_y_continuous(labels = human_numbers, limits=c(-0.5,22), breaks=seq(0, 22, 2)) +
  geom_bar(stat="identity", position=position_dodge()) +
  geom_errorbar(aes(ymin=lower, ymax=upper), width=.2,
                position=position_dodge(.9))+
  ggtitle("Durations in Pingpong") +
  labs(x="Interval", y="duration [µs]")
  
# tikz(file="figs/pingpong-timings.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/pingpong-timings.pdf", plot=pp_plot, width=8, height=5)

