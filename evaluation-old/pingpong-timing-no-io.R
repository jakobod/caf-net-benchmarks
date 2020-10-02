library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

source("evaluation/human_readable.R")

pingpong_net_master_full <- read.csv("evaluation/data/pingpong-timing-net.out", sep=",")
pingpong_net_master <- pingpong_net_master_full[,2:ncol(pingpong_net_master_full) - 1]
pingpong_net_master$avg <- rowMeans(pingpong_net_master[,2:ncol(pingpong_net_master)])
pingpong_net_master$sdev <- apply(pingpong_net_master[,2:ncol(pingpong_net_master)], 1, sd)
pingpong_net_master$backend <- 'net'
pingpong_net_master$upper <- pingpong_net_master$avg + pingpong_net_master$sdev
pingpong_net_master$lower <- pingpong_net_master$avg - pingpong_net_master$sdev

print(pingpong_net_master$avg)
print(pingpong_net_master$sdev)
print(pingpong_net_master$upper)
print(pingpong_net_master$lower)

pp_plot <- ggplot(pingpong_net_master, aes(x=what, y=avg, fill=backend)) + 
  scale_y_continuous(labels = human_numbers, limits=c(-1,22), breaks=seq(0, 22, 2)) +
  geom_bar(stat="identity", position=position_dodge()) +
  geom_errorbar(aes(ymin=lower, ymax=upper), width=.2,
                position=position_dodge(.9))+
  ggtitle("Durations in Pingpong") +
  labs(x="Interval", y="duration [µs]")
  
# tikz(file="figs/pingpong-timings.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/pingpong-timings-no-io.pdf", plot=pp_plot, width=8, height=5)

