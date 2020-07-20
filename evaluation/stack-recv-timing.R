library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

source("evaluation/human_readable.R")

net <- read.csv("evaluation/data/recv_basp_timing_processed.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
net$avg <- rowMeans(net[,2:48])
net$sdev <- apply(net[,2:48], 1, sd)
net$backend <- 'libcaf_net'

print(net$sdev)

ppdf <- rbind(net)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev

pp_plot <- ggplot(ppdf, aes(x = factor(num, level = c('read header','process header','read payload','process payload', 'deliver')), y = avg, fill=backend)) + 
  scale_y_continuous(labels = human_numbers, limits=c(0,65), breaks=seq(0, 65, 5)) +
  geom_bar(stat="identity", position=position_dodge()) +
  geom_errorbar(aes(ymin=lower, ymax=upper), width=.2,
                position=position_dodge(.9))+
  theme(legend.position = "none") +
  ggtitle("Durations of receiving a message") +
  labs(x="task", y="duration [µs]")


# tikz(file="figs/pingpong-timings.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/stack-timings-recv.pdf", plot=pp_plot, width=6, height=4)

