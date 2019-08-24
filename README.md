# Beat-and-Tempo-Tracking
Audio Beat  and Tempo Tracking

I wanted some beat and tempo tracking for my musical robotics projects. There are some things online, but I like knowing what is in the code so I can easily tweak it for my purposes. So I began implementing OBTAIN, described in this paper:

https://arxiv.org/pdf/1704.02216.pdf

The tempo-tracking part of OBTAIN borrows the autocorrelation and corss-correlation with pulses, as described in this paper,

https://ieeexplore.ieee.org/abstract/document/6879451

but with simple modifications to make is causal. This approach seems to be taken by most of the state-of-the-art tempo / beat trackers I have seen.

After estimating tempo, OBTAIN detects beats. The more I look at it, the less sense the beat-tracking part of OBTAIN makes. It is not my intention to be overly critical here, but I want to document the things I don't understand. 

They use two silmutaneous systems for tracking the beat. 

1) The first uses a multiscale peak-picking algorithm that is very good, but will never pick the most recent sample as a peak; yet this is how the authors say they are using it. This can be somewhat fixed with some simple modifications, but doing so will often pick the peak early. Because they only search for the beat within +- 10 samples of where the beat should be based on the tempo, this whole system just ends up being a regular pulse based on the tempo +-10 samples of jitter. Moreover, the tempo measurement will update many times before they look for the peak, and ultimately most tempo measurements get ignored. This all seems like too much given how computationally complex this algorithm is.

2) The second system uses another cross-correlation with pulses, which is straightforward enough. However, this beat phases replaces the one tracked by the first system when it has 'higher average values' than the first system. It isn't really clear what they mean by this.

There are a few other questionable things in the paper, but these make it un-implementable as described in the paper. There just isn't enough information.

So I'm committing this here so that it is preserved in its current state, which is as far as I could get with OBTAIN. Next, I'm going to look at another beat-tracking method to put on top of this and move away from OBTAIN. Ultamitely I will move this readme into a blog post and put a proper readme here.

------------------------------------------
Oh and another thing. I don't think beat and tempo tracking is a good research path for interactive musical robots. Most tempo tracking papers I have read just make the hypothesis that their method is one percent more accurate than someone elses method on some dataset. For musical robots, that isn't good enough. You need to be able to hypothesize that playing with a tempo-tracking robot is in some sense better or more fun or more engaging than playing with a non-tempo tracking robot. However I'm not sure that this will be the case. First is that state-of-the art tempo trackers are maddeningly frustrating to play with, both because they are always a few milliseconds behind, and because they are unstable. It makes a mistake, you try to correct, it over-corrects your correction, you over-over correct its over-correction, and the whole thing falls apart. Second is that I think what you usually want out of both robot and human players, is a very constant and stable tempo that does not really react to anything. I can't really think of a real musical situation where one person would be improvising tempo changes and someone else would be expected to follow them (unless it is a conductor, but we don't need to infer a conductor's beat phase from the music; in fact, often, for instance if you are standing on a huge stage behind and orchestra, if you go by what you hear you will be late because of the slow speed of sound). Even in small ensembles, and even in jazz, in my experience, tempo changes are always very thoroughly rehearsed. This would be the equivalent of programming in the desired tempo curve in the measures where it belongs. So I think the best approach overall for robots is to set the tempo once (the human could do this by tapping a few pulses) and then leave it alone. I basically think tempo tracking is the invention of engineers who live a box and just want to explore the mathematics without thinking too much about whether it is really a good idea.

End of rant.
