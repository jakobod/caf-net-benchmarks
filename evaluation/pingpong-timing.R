library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

source("evaluation/human_readable.R")

pingpong_net_master <- read.csv("evaluation/data/pingpong-timings.out", sep=",")
pingpong_net_master$avg <- rowMeans(pingpong_net_master[,2:4])
pingpong_net_master$sdev <- apply(pingpong_net_master, 1, sd)
pingpong_net_master$proto <- '1 net - master'

print(pingpong_net_master$avg)

ppdf <- rbind(pingpong_net_master)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev

pp_plot <- ggplot(ppdf, aes(x=which, y=avg)) + 
  scale_y_continuous(labels = human_numbers, limits=c(0,170), breaks=seq(0, 170, 10)) +
  geom_bar(stat = 'identity', position='dodge') +
  geom_errorbar(aes(ymin=lower, ymax=upper), width=.2,
                position=position_dodge(.9))
  
tikz(file="figs/pingpong-timings.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/pingpong-timings.pdf", plot=pp_plot, width=8, height=5)

