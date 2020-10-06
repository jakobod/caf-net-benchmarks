library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)
require(scales)

source("evaluation/human_readable.R")


pingpong_io <- read.csv("evaluation/out/pingpong-tcp-io-amount-pings.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
pingpong_io$avg <- rowMeans(pingpong_io[,2:11])
pingpong_io$avg <- pingpong_io$avg/1000
pingpong_io$sdev <- apply(pingpong_io[,2:11], 1, sd)
pingpong_io$sdev <- pingpong_io$sdev/1000
pingpong_io$proto <- 'libcaf_io - tcp'

pingpong_net <- read.csv("evaluation/out/pingpong-tcp-net-amount-pings.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
pingpong_net$avg <- rowMeans(pingpong_net[,2:11])
pingpong_net$avg <- pingpong_net$avg/1000
pingpong_net$sdev <- apply(pingpong_net[,2:11], 1, sd)
pingpong_net$sdev <- pingpong_net$sdev/1000
pingpong_net$proto <- 'libcaf_net - tcp'

ppdf <- rbind(pingpong_io, pingpong_net)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev


pp_plot_log <- ggplot(ppdf, aes(x=num_pings, y=avg, color=proto)) +
  geom_line() + # size=0.8) +
  geom_point(aes(shape=proto), size = 1, stroke=0.6) +
  geom_errorbar(
    mapping=aes(
      ymin=lower,
      ymax=upper
    ),
    width=0.4
  ) +
  scale_x_continuous(trans = log2_trans(),
                     breaks = trans_breaks("log2", function(x) 2^x),
                     labels = trans_format("log2", math_format(2^.x))) +
  scale_y_continuous(trans = log2_trans(),
                     breaks = trans_breaks("log2", function(x) 2^x),
                     labels = trans_format("log2", math_format(2^.x))) +
  theme_bw() +
  theme(
    legend.title=element_blank(),
    legend.key=element_rect(fill='white'), 
    legend.background=element_rect(fill="white", colour="black", size=0.25),
    legend.direction="vertical",
    legend.justification=c(0, 1),
    legend.position=c(0,1),
    legend.box.margin=margin(c(3, 3, 3, 3)),
    legend.key.height=unit(0.4,"line"),
    legend.key.size=unit(0.6, 'lines'),
    text=element_text(size=9),
    strip.text.x=element_blank()
  ) +
  scale_color_brewer(type="qual", palette=6) +
  scale_fill_brewer(palette="Dark2") +
  ggtitle("Pingpong") +
  labs(x="roundtrips [#]", y="duration [ms]")

pp_plot <- ggplot(ppdf, aes(x=num_pings, y=avg, color=proto)) +
  geom_line() + # size=0.8) +
  geom_point(aes(shape=proto), size = 1, stroke=0.6) +
  geom_errorbar(
    mapping=aes(
      ymin=lower,
      ymax=upper
    ),
    width=0.4
  ) +
  scale_x_continuous() +
  scale_y_continuous() +
  theme_bw() +
  theme(
    legend.title=element_blank(),
    legend.key=element_rect(fill='white'), 
    legend.background=element_rect(fill="white", colour="black", size=0.25),
    legend.direction="vertical",
    legend.justification=c(0, 1),
    legend.position=c(0,1),
    legend.box.margin=margin(c(3, 3, 3, 3)),
    legend.key.height=unit(0.4,"line"),
    legend.key.size=unit(0.6, 'lines'),
    text=element_text(size=9),
    strip.text.x=element_blank()
  ) +
  scale_color_brewer(type="qual", palette=6) +
  scale_fill_brewer(palette="Dark2") +
  ggtitle("Pingpong") +
  labs(x="roundtrips [#]", y="duration [ms]")

tikz(file="tikz/pingpong.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

#ggsave("figs/pingpong.pdf", plot=pp_plot, width=3.4, height=2.3)
ggsave("figs/pingpong-log-scale.pdf", plot=pp_plot_log, width=4, height=3)
ggsave("figs/pingpong.pdf", plot=pp_plot, width=4, height=3)

