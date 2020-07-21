library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

source("evaluation/human_readable.R")

net <- read.csv("evaluation/data/recv_basp_timing_processed.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
net$avg <- rowMeans(net[,2:62])
net$sdev <- apply(net[,2:62], 1, sd)
net$backend <- 'libcaf_net'

net_workers <- read.csv("evaluation/data/recv_basp_timing_0_workers_processed.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
net_workers$avg <- rowMeans(net_workers[,2:62])
net_workers$sdev <- apply(net_workers[,2:62], 1, sd)
net_workers$backend <- 'libcaf_net_0_workers'

print(net$sdev)

ppdf <- rbind(net, net_workers)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev

mylabels <- c('read', 'basp', 'process', 'deliver')

pp_plot <- ggplot(ppdf, aes(x = num, y = avg, fill=backend)) + 
  scale_x_discrete(labels = mylabels) +
  scale_y_continuous(labels = human_numbers, limits=c(0,65), breaks=seq(0, 65, 10)) +
  geom_bar(stat="identity", position=position_dodge()) +
  geom_errorbar(aes(ymin=lower, ymax=upper), width=.2,
                position=position_dodge(.9)) +
  theme(text = element_text(size=27), legend.position = "none") +
  labs(x="task", y="duration [µs]")


# tikz(file="figs/pingpong-timings.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/stack-timings-recv-both.pdf", plot=pp_plot, width=9, height=5)

