library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)
require(scales)

source("evaluation/human_readable.R")

streaming_net <- read.csv("evaluation/out/blank-streaming-net-message-size.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
streaming_net$avg <- rowMeans(streaming_net[,2:11])
streaming_net$avg <- streaming_net$avg/1000
streaming_net$sdev <- apply(streaming_net[,2:11], 1, sd)
streaming_net$sdev <- streaming_net$sdev/1000
streaming_net$proto <- 'libcaf_net'

streaming_io <- read.csv("evaluation/out/blank-streaming-io-message-size.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
streaming_io$avg <- rowMeans(streaming_io[,2:11])
streaming_io$avg <- streaming_io$avg/1000
streaming_io$sdev <- apply(streaming_io[,2:11], 1, sd)
streaming_io$sdev <- streaming_io$sdev/1000
streaming_io$proto <- 'libcaf_io'

ppdf <- rbind(streaming_net, streaming_io)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev

pp_plot <- ggplot(ppdf, aes(x=message_size, y=avg, color=proto)) +
  geom_line() + # size=0.8) +
  geom_point(aes(shape=proto), size = 2, stroke=0.8) +
  geom_errorbar(
    mapping=aes(
      ymin=lower,
      ymax=upper
    ),
    width=0.2
  ) +
  #scale_x_continuous(trans = log2_trans(),
  #                   breaks = trans_breaks("log2", function(x) 2^x),
  #                   labels = trans_format("log2", math_format(2^.x))) +
  #scale_y_continuous(trans = log2_trans(),
  #                   breaks = trans_breaks("log2", function(x) 2^x),
  #                   labels = trans_format("log2", math_format(2^.x))) +
  scale_x_continuous(trans = log2_trans(),
                     breaks = trans_breaks("log2", function(x) 2^x),
                     labels = trans_format("log2", math_format(2^.x))) +
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
  ggtitle("Streaming 100 MiB") +
  labs(x="message size [Byte]", y="duration [ms]")

tikz(file="tikz/blank_streaming_message_sizes.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/blank_streaming_message_sizes.pdf", plot=pp_plot, width=4, height=3)

