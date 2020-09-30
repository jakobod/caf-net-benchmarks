library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

source("evaluation/human_readable.R")

net <- read.csv("evaluation/data/send_basp_timing_processed.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
net$avg <- rowMeans(net[,2:69])
net$sdev <- apply(net[,2:69], 1, sd)
net$backend <- 'libcaf_net'

print(net$sdev)

ppdf <- rbind(net)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev

mylabels <- c('send','enqueue','event','dequeue', 'process', 'write')

pp_plot <- ggplot(ppdf, aes(x = num, y = avg, fill=backend)) + 
  scale_x_discrete(labels = mylabels) +
  scale_y_continuous(labels = human_numbers, limits=c(0,65), breaks=seq(0, 65, 10)) +
  geom_bar(stat="identity", position=position_dodge()) +
  geom_errorbar(aes(ymin=lower, ymax=upper), width=.2,
                position=position_dodge(.9))+
  theme(text = element_text(size=27), legend.position = "none") +
  labs(x="task", y="duration [µs]")

pp_plot
dev.off()

ggsave("figs/stack-timings-send.pdf", plot=pp_plot, width=9, height=5)

